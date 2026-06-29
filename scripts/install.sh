#!/usr/bin/env bash
# CopyClip installer / uninstaller.
#
# Detects your distribution and CPU architecture, downloads the best-matching
# release asset, and installs it: a native package (.deb/.rpm) when possible,
# otherwise a self-contained AppImage, otherwise a source build. Re-running on a
# system that already has CopyClip offers to update or remove it.
#
#   curl -fsSL https://raw.githubusercontent.com/Walkercito/CopyClip/main/scripts/install.sh | bash
#   curl -fsSL .../install.sh | bash -s -- --uninstall
set -euo pipefail

REPO="Walkercito/CopyClip"
API="https://api.github.com/repos/${REPO}/releases/latest"
APP="copyclip"
APP_ID="dev.walkercito.CopyClip"
USER_BIN="${HOME}/.local/bin"
USER_APPS="${HOME}/.local/share/applications"
USER_ICONS="${HOME}/.local/share/icons/hicolor/scalable/apps"

# --- pretty output ------------------------------------------------------------
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
  BOLD=$'\033[1m'; DIM=$'\033[2m'; RED=$'\033[31m'; GRN=$'\033[32m'
  YLW=$'\033[33m'; BLU=$'\033[34m'; CYN=$'\033[36m'; NC=$'\033[0m'
else
  BOLD=""; DIM=""; RED=""; GRN=""; YLW=""; BLU=""; CYN=""; NC=""
fi
info() { printf '%s\n' "${BLU}${BOLD}::${NC} ${BOLD}$*${NC}"; }
step() { printf '%s\n' "   ${CYN}→${NC} $*"; }
ok()   { printf '%s\n' "   ${GRN}✓${NC} $*"; }
warn() { printf '%s\n' "   ${YLW}!${NC} $*"; }
die()  { printf '%s\n' "   ${RED}✗${NC} $*" >&2; exit 1; }

banner() {
  printf '%s\n' ""
  printf '%s\n' "${CYN}${BOLD}   ┌─────────────────────────────────────┐${NC}"
  printf '%s\n' "${CYN}${BOLD}   │${NC}  📋  ${BOLD}CopyClip${NC}  ·  Linux installer    ${CYN}${BOLD}│${NC}"
  printf '%s\n' "${CYN}${BOLD}   └─────────────────────────────────────┘${NC}"
  printf '%s\n' ""
}

usage() {
  banner
  cat <<EOF
${BOLD}Usage${NC}
  install.sh [option]

${BOLD}Options${NC}
  ${GRN}-i${NC}, ${GRN}--install${NC}      Install, or update an existing install, to the latest release
  ${GRN}-u${NC}, ${GRN}--uninstall${NC}    Remove CopyClip
  ${GRN}-h${NC}, ${GRN}--help${NC}         Show this help

With no option, CopyClip is installed — or, if it is already present, you are
asked whether to update or remove it.

${BOLD}Examples${NC}
  ${DIM}# install or update${NC}
  curl -fsSL https://raw.githubusercontent.com/${REPO}/main/scripts/install.sh | bash
  ${DIM}# uninstall${NC}
  curl -fsSL https://raw.githubusercontent.com/${REPO}/main/scripts/install.sh | bash -s -- -u
EOF
}

need() { command -v "$1" >/dev/null 2>&1; }

require_tools() {
  need curl || die "curl is required but not installed."
  need uname || die "uname is required but not installed."
}

# --- platform detection -------------------------------------------------------
detect_arch() {
  case "$(uname -m)" in
    x86_64 | amd64) ARCH=x86_64; DEB_ARCH=amd64 ;;
    aarch64 | arm64) ARCH=aarch64; DEB_ARCH=arm64 ;;
    *) ARCH="$(uname -m)"; DEB_ARCH="" ;;
  esac
}

detect_distro() {
  local id="" like=""
  if [ -r /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    id="${ID:-}"; like="${ID_LIKE:-}"
  fi
  case " ${id} ${like} " in
    *" debian "* | *" ubuntu "*) FAMILY=debian ;;
    *" fedora "* | *" rhel "* | *" centos "*) FAMILY=fedora ;;
    *" suse "* | *" opensuse "*) FAMILY=suse ;;
    *" arch "*) FAMILY=arch ;;
    *) FAMILY=unknown ;;
  esac
  PRETTY_OS="${PRETTY_NAME:-${id:-Linux}}"
}

# Set $SUDO to "sudo" (or "" when already root). Fails if neither is possible.
ensure_sudo() {
  if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
  elif need sudo; then
    SUDO="sudo"
  else
    die "Root privileges are needed and 'sudo' is not available. Re-run as root."
  fi
}

# --- installed-state detection ------------------------------------------------
# Sets INSTALLED_VIA (""|deb|rpm|appimage) and INSTALLED_VERSION.
detect_installed() {
  INSTALLED_VIA=""; INSTALLED_VERSION=""
  if need dpkg && dpkg -s "$APP" >/dev/null 2>&1; then
    INSTALLED_VIA=deb
    INSTALLED_VERSION="$(dpkg-query -W -f='${Version}' "$APP" 2>/dev/null || true)"
  elif need rpm && rpm -q "$APP" >/dev/null 2>&1; then
    INSTALLED_VIA=rpm
    INSTALLED_VERSION="$(rpm -q --qf '%{VERSION}' "$APP" 2>/dev/null || true)"
  elif [ -e "${USER_BIN}/${APP}" ]; then
    INSTALLED_VIA=appimage
  fi
}

# --- release metadata ---------------------------------------------------------
fetch_release() {
  step "Querying the latest release…"
  RELEASE_JSON="$(curl -fsSL "$API")" \
    || die "Could not reach GitHub. Check your internet connection."
  RELEASE_TAG="$(printf '%s' "$RELEASE_JSON" \
    | grep -oE '"tag_name"[[:space:]]*:[[:space:]]*"[^"]+"' | head -1 \
    | sed -E 's/.*"([^"]+)"$/\1/')"
  [ -n "$RELEASE_TAG" ] || die "No published release found for ${REPO} yet."
}

# Download URL of the first asset whose filename contains $1 (empty if none).
asset_url() {
  printf '%s' "$RELEASE_JSON" \
    | grep -oE '"browser_download_url"[[:space:]]*:[[:space:]]*"[^"]+"' \
    | sed -E 's/.*"(https:[^"]+)".*/\1/' \
    | grep -F -- "$1" | head -1
}

download() {
  step "Downloading $(basename "$2")…"
  curl -fL --progress-bar -o "$2" "$1" || die "Download failed: $1"
}

# Remove a previous user-level AppImage install so it can't shadow a package one.
purge_appimage_files() {
  if [ -e "${USER_BIN}/${APP}" ] || [ -e "${USER_APPS}/${APP_ID}.desktop" ]; then
    step "Removing a previous AppImage install to avoid a conflict…"
    rm -f "${USER_BIN}/${APP}" "${USER_APPS}/${APP_ID}.desktop" "${USER_ICONS}/${APP_ID}.svg"
    need update-desktop-database && update-desktop-database -q "$USER_APPS" 2>/dev/null || true
  fi
}

finish_hint() {
  printf '%s\n' ""
  ok "Done! Find ${BOLD}CopyClip${NC} in your applications menu, or run ${BOLD}${APP}${NC}."
}

# --- install methods (return non-zero to fall through to the next) ------------
install_deb() {
  local url; url="$(asset_url "_${DEB_ARCH}.deb")"
  [ -n "$url" ] || return 1
  local f="${TMP}/${APP}.deb"
  download "$url" "$f"
  ensure_sudo
  step "Installing with apt (dependencies resolved automatically)…"
  $SUDO apt-get install -y "$f" || return 1
  purge_appimage_files
  ok "Installed ${APP} ${RELEASE_TAG} via apt."
}

install_rpm() {
  local url; url="$(asset_url ".${ARCH}.rpm")"
  [ -n "$url" ] || return 1
  local f="${TMP}/${APP}.rpm"
  download "$url" "$f"
  ensure_sudo
  if [ "$FAMILY" = suse ]; then
    step "Installing with zypper…"
    $SUDO zypper --non-interactive install --allow-unsigned-rpm "$f" || return 1
  else
    step "Installing with dnf…"
    $SUDO dnf install -y "$f" || return 1
  fi
  purge_appimage_files
  ok "Installed ${APP} ${RELEASE_TAG}."
}

install_appimage() {
  local url; url="$(asset_url "-${ARCH}.AppImage")"
  [ -n "$url" ] || return 1
  mkdir -p "$USER_BIN" "$USER_APPS" "$USER_ICONS"
  local f="${USER_BIN}/${APP}"
  download "$url" "$f"
  chmod +x "$f"

  # Pull the bundled icon out of the AppImage for the menu entry (best-effort).
  ( cd "$TMP" && "$f" --appimage-extract \
      "usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg" >/dev/null 2>&1 ) || true
  local icon="${TMP}/squashfs-root/usr/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
  [ -f "$icon" ] && cp "$icon" "${USER_ICONS}/${APP_ID}.svg"

  cat > "${USER_APPS}/${APP_ID}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=CopyClip
Comment=Keep and reuse your clipboard history
Exec=${f}
Icon=${APP_ID}
Terminal=false
Categories=Utility;GTK;
StartupWMClass=${APP_ID}
EOF
  need update-desktop-database && update-desktop-database -q "$USER_APPS" 2>/dev/null || true
  ok "Installed ${APP} ${RELEASE_TAG} to ${USER_BIN} (AppImage)."
  case ":${PATH}:" in
    *":${USER_BIN}:"*) ;;
    *) warn "${USER_BIN} is not on your PATH — add it to launch '${APP}' from a terminal." ;;
  esac
}

install_from_source() {
  warn "No prebuilt binary fits this system; building from source."
  for t in git cmake; do need "$t" || die "Building from source needs '$t'. Install it and retry."; done
  info "Compiling CopyClip — this can take several minutes…"
  local src="${TMP}/src"
  step "Cloning ${REPO}…"
  git clone --depth 1 "https://github.com/${REPO}.git" "$src" || die "git clone failed."
  export VCPKG_ROOT="${src}/vcpkg"
  step "Bootstrapping vcpkg…"
  git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" >/dev/null 2>&1 || true
  "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics || die "vcpkg bootstrap failed."
  step "Configuring and building (release)…"
  ( cd "$src" && cmake --preset release && cmake --build --preset release --target copyclip-gtk ) \
    || die "Build failed. See the output above."
  ensure_sudo
  step "Installing to /usr/local…"
  $SUDO cmake --install "${src}/build/release" --prefix /usr/local || die "Install failed."
  ok "Built and installed ${APP} from source."
}

do_install() {
  fetch_release
  TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
  info "Installing CopyClip ${RELEASE_TAG} for ${ARCH} on ${PRETTY_OS}…"

  if [ -z "$DEB_ARCH" ]; then # unsupported arch → only source builds
    install_from_source
    finish_hint
    return
  fi

  local native_ok=1
  case "$FAMILY" in
    debian) install_deb || native_ok=0 ;;
    fedora | suse) install_rpm || native_ok=0 ;;
    *) native_ok=0 ;; # arch/unknown have no native package → AppImage
  esac

  if [ "$native_ok" -eq 0 ]; then
    case "$FAMILY" in
      debian | fedora | suse) warn "Native package unavailable; falling back to the AppImage." ;;
    esac
    install_appimage || install_from_source
  fi
  finish_hint
}

do_uninstall() {
  detect_installed
  if [ -z "$INSTALLED_VIA" ]; then
    warn "CopyClip does not appear to be installed."
    exit 0
  fi
  info "Removing CopyClip (installed via ${INSTALLED_VIA})…"
  case "$INSTALLED_VIA" in
    deb)
      ensure_sudo
      $SUDO apt-get remove -y "$APP" || die "Removal failed."
      ;;
    rpm)
      ensure_sudo
      if [ "$FAMILY" = suse ]; then
        $SUDO zypper --non-interactive remove "$APP" || die "Removal failed."
      else
        $SUDO dnf remove -y "$APP" || die "Removal failed."
      fi
      ;;
    appimage)
      rm -f "${USER_BIN}/${APP}" "${USER_APPS}/${APP_ID}.desktop" "${USER_ICONS}/${APP_ID}.svg"
      need update-desktop-database && update-desktop-database -q "$USER_APPS" 2>/dev/null || true
      ;;
  esac
  ok "CopyClip removed."
}

# --- entry point --------------------------------------------------------------
main() {
  local action=""
  case "${1:-}" in
    -i | --install) action=install ;;
    -u | --uninstall) action=uninstall ;;
    -h | --help) usage; exit 0 ;;
    "") action="" ;;
    *) usage; die "Unknown option: $1" ;;
  esac

  require_tools
  detect_arch
  detect_distro

  case "$action" in
    install) banner; do_install ;;
    uninstall) banner; do_uninstall ;;
    "")
      banner
      detect_installed
      if [ -n "$INSTALLED_VIA" ]; then
        info "CopyClip is already installed${INSTALLED_VERSION:+ (v${INSTALLED_VERSION})}."
        printf '%s' "   ${BOLD}[U]${NC}pdate, ${BOLD}[r]${NC}emove, or ${BOLD}[c]${NC}ancel? "
        local ans=""
        read -r ans </dev/tty 2>/dev/null || ans=""
        case "$ans" in
          "" | u | U) do_install ;;
          r | R) do_uninstall ;;
          *) info "Cancelled."; exit 0 ;;
        esac
      else
        do_install
      fi
      ;;
  esac
}

main "$@"
