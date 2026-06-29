#!/usr/bin/env bash
# CopyClip installer / uninstaller.
#
# Detects your distribution and CPU architecture, downloads the best-matching
# release asset, and installs it: a native package (.deb/.rpm) when possible,
# otherwise a self-contained AppImage, otherwise a source build. CopyClip is added
# to your login autostart. Re-running offers to update or remove it.
#
#   curl -fsSL https://raw.githubusercontent.com/Walkercito/CopyClip/main/scripts/install.sh | bash
#   curl -fsSL .../install.sh | bash -s -- --uninstall
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

REPO="Walkercito/CopyClip"
API="https://api.github.com/repos/${REPO}/releases/latest"
APP="copyclip"
APP_ID="dev.walkercito.CopyClip"
USER_BIN="${HOME}/.local/bin"
USER_APPS="${HOME}/.local/share/applications"
USER_ICONS="${HOME}/.local/share/icons/hicolor/scalable/apps"
AUTOSTART_DIR="${XDG_CONFIG_HOME:-${HOME}/.config}/autostart"
AUTOSTART_FILE="${AUTOSTART_DIR}/${APP_ID}.desktop"

# --- presentation -------------------------------------------------------------
if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
  BOLD=$'\033[1m'; DIM=$'\033[2m'; RED=$'\033[31m'; GRN=$'\033[32m'
  YLW=$'\033[33m'; BLU=$'\033[34m'; CYN=$'\033[36m'; MAG=$'\033[35m'; WHT=$'\033[97m'; NC=$'\033[0m'
else
  BOLD=""; DIM=""; RED=""; GRN=""; YLW=""; BLU=""; CYN=""; MAG=""; WHT=""; NC=""
fi

clear_screen() { if [ -t 1 ]; then printf '\033[2J\033[3J\033[H'; fi; }

headline() {
  printf '\n'
  while IFS= read -r line; do
    printf '  %s%s%s%s\n' "$CYN" "$BOLD" "$line" "$NC"
  done <<'EOF'
░█▀▀░█▀█░█▀█░█░█░█▀▀░█░░░▀█▀░█▀█
░█░░░█░█░█▀▀░░█░░█░░░█░░░░█░░█▀▀
░▀▀▀░▀▀▀░▀░░░░▀░░▀▀▀░▀▀▀░▀▀▀░▀░░
EOF
  printf '  %sclipboard history for Linux%s\n' "$DIM" "$NC"
}
section() { printf '\n%s%s::%s %s%s%s\n' "$BOLD" "$BLU" "$NC" "$BOLD" "$*" "$NC"; }
field()   { printf '   %s%-14s%s %s\n' "$DIM" "$1" "$NC" "$2"; }
tick()    { printf '   %s✓%s %s\n' "$GRN" "$NC" "$*"; }
note()    { printf '   %s•%s %s\n' "$CYN" "$NC" "$*"; }
warn()    { printf '   %s!%s %s\n' "$YLW" "$NC" "$*"; }
die()     { printf '   %s✗%s %s\n' "$RED" "$NC" "$*" >&2; exit 1; }

# Run a command behind a spinner, surfacing its output only if it fails.
# Returns the command's exit status.
run_step() {
  local msg="$1"; shift
  local log rc=0
  log="$(mktemp)"
  if [ -t 1 ]; then
    ( "$@" ) >"$log" 2>&1 &
    local pid=$! frames='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏' i=0
    while kill -0 "$pid" 2>/dev/null; do
      i=$(((i + 1) % 10))
      printf '\r   %s%s%s %s' "$CYN" "${frames:$i:1}" "$NC" "$msg"
      sleep 0.1
    done
    wait "$pid" || rc=$?
    printf '\r\033[2K'
  else
    ( "$@" ) >"$log" 2>&1 || rc=$?
  fi
  if [ "$rc" -eq 0 ]; then
    tick "$msg"
  else
    printf '   %s✗%s %s\n' "$RED" "$NC" "$msg" >&2
    sed 's/^/       /' "$log" | tail -n 8 >&2
  fi
  rm -f "$log"
  return "$rc"
}

usage() {
  headline
  cat <<EOF

${BOLD}Usage${NC}
  install.sh [option]

${BOLD}Options${NC}
  ${GRN}-i${NC}, ${GRN}--install${NC}      Install, or update an existing install, to the latest release
  ${GRN}-u${NC}, ${GRN}--uninstall${NC}    Remove CopyClip
  ${GRN}-h${NC}, ${GRN}--help${NC}         Show this help

With no option, CopyClip is installed — or, if already present, you are asked
whether to update or remove it.

${BOLD}Examples${NC}
  ${DIM}curl -fsSL https://raw.githubusercontent.com/${REPO}/main/scripts/install.sh | bash${NC}
  ${DIM}curl -fsSL .../install.sh | bash -s -- -u${NC}
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

# Set $SUDO ("sudo" or "") and pre-authenticate so later steps run unattended.
ensure_sudo() {
  if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
    return
  fi
  need sudo || die "Root privileges are needed and 'sudo' is not available. Re-run as root."
  SUDO="sudo"
  note "Administrator access is required to install the package."
  sudo -v </dev/tty || die "Could not obtain administrator access."
}

# --- installed-state detection ------------------------------------------------
detect_installed() {
  INSTALLED_VIA=""; INSTALLED_VERSION=""
  if need dpkg && [ "$(dpkg-query -W -f='${db:Status-Status}' "$APP" 2>/dev/null)" = installed ]; then
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

# --- autostart ----------------------------------------------------------------
enable_autostart() { # $1 = exec command/path
  mkdir -p "$AUTOSTART_DIR"
  cat > "$AUTOSTART_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=CopyClip
Comment=Keep and reuse your clipboard history
Exec=$1
Icon=${APP_ID}
Terminal=false
X-GNOME-Autostart-enabled=true
EOF
}

disable_autostart() { rm -f "$AUTOSTART_FILE"; }

# Remove a previous user-level AppImage install so it can't shadow a package one.
purge_appimage_files() {
  if [ -e "${USER_BIN}/${APP}" ] || [ -e "${USER_APPS}/${APP_ID}.desktop" ]; then
    rm -f "${USER_BIN}/${APP}" "${USER_APPS}/${APP_ID}.desktop" "${USER_ICONS}/${APP_ID}.svg"
    need update-desktop-database && update-desktop-database -q "$USER_APPS" 2>/dev/null || true
    note "Removed a previous AppImage install to avoid a conflict."
  fi
}

# --- install methods (set INSTALLED_BIN; return non-zero to fall through) ------
install_deb() {
  local url; url="$(asset_url "_${DEB_ARCH}.deb")"
  [ -n "$url" ] || return 1
  ensure_sudo
  local f="${TMP}/${APP}.deb"
  run_step "Downloading $(basename "$url")" curl -fsSL -o "$f" "$url" || die "Download failed."
  run_step "Installing with apt" $SUDO apt-get install -y "$f" || return 1
  purge_appimage_files
  INSTALLED_BIN="/usr/bin/${APP}"
}

install_rpm() {
  local url; url="$(asset_url ".${ARCH}.rpm")"
  [ -n "$url" ] || return 1
  ensure_sudo
  local f="${TMP}/${APP}.rpm"
  run_step "Downloading $(basename "$url")" curl -fsSL -o "$f" "$url" || die "Download failed."
  if [ "$FAMILY" = suse ]; then
    run_step "Installing with zypper" \
      $SUDO zypper --non-interactive install --allow-unsigned-rpm "$f" || return 1
  else
    run_step "Installing with dnf" $SUDO dnf install -y "$f" || return 1
  fi
  purge_appimage_files
  INSTALLED_BIN="/usr/bin/${APP}"
}

install_appimage() {
  local url; url="$(asset_url "-${ARCH}.AppImage")"
  [ -n "$url" ] || return 1
  mkdir -p "$USER_BIN" "$USER_APPS" "$USER_ICONS"
  local f="${USER_BIN}/${APP}"
  run_step "Downloading $(basename "$url")" curl -fsSL -o "$f" "$url" || die "Download failed."
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
  tick "Installed to ${USER_BIN/#$HOME/\~}"
  INSTALLED_BIN="$f"
  case ":${PATH}:" in
    *":${USER_BIN}:"*) ;;
    *) note "Add ${USER_BIN/#$HOME/\~} to your PATH to launch '${APP}' from a terminal." ;;
  esac
}

install_from_source() {
  warn "No prebuilt binary fits this system; building from source (several minutes)."
  for t in git cmake; do need "$t" || die "Building from source needs '$t'. Install it and retry."; done
  local src="${TMP}/src"
  run_step "Cloning ${REPO}" git clone --depth 1 "https://github.com/${REPO}.git" "$src" \
    || die "git clone failed."
  export VCPKG_ROOT="${src}/vcpkg"
  run_step "Bootstrapping vcpkg" bash -c \
    "git clone --depth 1 https://github.com/microsoft/vcpkg.git '$VCPKG_ROOT' && '$VCPKG_ROOT/bootstrap-vcpkg.sh' -disableMetrics" \
    || die "vcpkg bootstrap failed."
  run_step "Compiling (release)" bash -c \
    "cd '$src' && cmake --preset release && cmake --build --preset release --target copyclip-gtk" \
    || die "Build failed."
  ensure_sudo
  run_step "Installing to /usr/local" \
    $SUDO cmake --install "${src}/build/release" --prefix /usr/local || die "Install failed."
  INSTALLED_BIN="/usr/local/bin/${APP}"
}

# True when $1 is an older version than $2 (i.e. an update is available).
version_lt() {
  [ "$1" != "$2" ] &&
    [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V 2>/dev/null | head -1)" = "$1" ]
}

# Color each character of $1 across a rainbow palette (bold).
rainbow() {
  local s="$1" out="" i
  local palette=("$RED" "$YLW" "$GRN" "$CYN" "$BLU" "$MAG")
  for ((i = 0; i < ${#s}; i++)); do
    out+="${palette[i % 6]}${s:i:1}"
  done
  printf '%s%s%s' "$BOLD" "$out" "$NC"
}

do_install() {
  [ -n "${RELEASE_JSON:-}" ] || fetch_release
  TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

  section "System"
  field "Distribution" "$PRETTY_OS"
  field "Architecture" "$ARCH"
  field "Release" "$RELEASE_TAG"

  section "Installing CopyClip ${RELEASE_TAG}"
  INSTALLED_BIN="$APP"
  if [ -z "$DEB_ARCH" ]; then
    install_from_source
  else
    local native_ok=1
    case "$FAMILY" in
      debian) install_deb || native_ok=0 ;;
      fedora | suse) install_rpm || native_ok=0 ;;
      *) native_ok=0 ;;
    esac
    if [ "$native_ok" -eq 0 ]; then
      case "$FAMILY" in
        debian | fedora | suse) warn "Native package unavailable; falling back to the AppImage." ;;
      esac
      install_appimage || install_from_source
    fi
  fi

  enable_autostart "$INSTALLED_BIN"
  tick "Enabled autostart on login"

  section "Done"
  tick "CopyClip ${RELEASE_TAG} is installed"
  note "Find it in your applications menu, or launch it from a terminal:"
  printf '       %s$%s %s%s%s%s\n\n' "$DIM" "$NC" "$BOLD" "$WHT" "$APP" "$NC"
}

do_update() {
  fetch_release
  local latest="${RELEASE_TAG#v}"
  if [ -n "$INSTALLED_VERSION" ]; then
    if ! version_lt "$INSTALLED_VERSION" "$latest"; then
      section "Already up to date"
      tick "You have the latest version (v${INSTALLED_VERSION})."
      printf '\n'
      exit 0
    fi
    printf '\n   %s available — %sv%s%s  %s(you have v%s)%s\n' \
      "$(rainbow Update)" "$BOLD" "$latest" "$NC" "$DIM" "$INSTALLED_VERSION" "$NC"
  fi
  do_install
}

do_uninstall() {
  detect_installed
  if [ -z "$INSTALLED_VIA" ]; then
    section "Uninstall"
    warn "CopyClip does not appear to be installed."
    printf '\n'
    exit 0
  fi
  local via_label="$INSTALLED_VIA"
  case "$INSTALLED_VIA" in
    deb) via_label="system package (apt)" ;;
    rpm) via_label="system package (rpm)" ;;
    appimage) via_label="AppImage" ;;
  esac
  section "Removing CopyClip"
  field "Version" "${INSTALLED_VERSION:-—}"
  field "Method" "$via_label"

  case "$INSTALLED_VIA" in
    deb)
      ensure_sudo
      run_step "Uninstalling with apt" $SUDO apt-get purge -y "$APP" || die "Removal failed."
      ;;
    rpm)
      ensure_sudo
      if [ "$FAMILY" = suse ]; then
        run_step "Uninstalling with zypper" \
          $SUDO zypper --non-interactive remove "$APP" || die "Removal failed."
      else
        run_step "Uninstalling with dnf" $SUDO dnf remove -y "$APP" || die "Removal failed."
      fi
      ;;
    appimage)
      rm -f "${USER_BIN}/${APP}" "${USER_APPS}/${APP_ID}.desktop" "${USER_ICONS}/${APP_ID}.svg"
      need update-desktop-database && update-desktop-database -q "$USER_APPS" 2>/dev/null || true
      tick "Removed AppImage and menu entry"
      ;;
  esac
  disable_autostart
  tick "Removed autostart entry"

  section "Done"
  tick "CopyClip removed"
  printf '\n'
}

# --- entry point --------------------------------------------------------------
main() {
  local action=""
  case "${1:-}" in
    -i | --install) action=install ;;
    -u | --uninstall) action=uninstall ;;
    -h | --help) usage; exit 0 ;;
    "") action="" ;;
    *) usage; printf '\n'; die "Unknown option: $1" ;;
  esac

  require_tools
  detect_arch
  detect_distro
  clear_screen
  headline

  case "$action" in
    install)
      detect_installed
      if [ -n "$INSTALLED_VIA" ]; then do_update; else do_install; fi
      ;;
    uninstall) do_uninstall ;;
    "")
      detect_installed
      if [ -n "$INSTALLED_VIA" ]; then
        section "CopyClip is already installed${INSTALLED_VERSION:+ (v${INSTALLED_VERSION})}"
        printf '   %s[%sU%s]%spdate, %s[%sR%s]%semove, or %s[%sC%s]%sancel? ' \
          "$WHT" "$GRN" "$WHT" "$NC" "$WHT" "$RED" "$WHT" "$NC" "$WHT" "$YLW" "$WHT" "$NC"
        local ans=""
        read -rsn1 ans </dev/tty 2>/dev/null || ans=""
        printf '%s\n' "$ans"
        case "$ans" in
          "" | u | U) do_update ;;
          r | R) do_uninstall ;;
          *) note "Cancelled."; printf '\n'; exit 0 ;;
        esac
      else
        do_install
      fi
      ;;
  esac
}

main "$@"
