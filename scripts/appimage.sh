#!/usr/bin/env bash
# Build a self-contained AppImage of the CopyClip GTK app from an already-built
# release tree, bundling the GTK4/libadwaita runtime via linuxdeploy-plugin-gtk.
#
# Usage: appimage.sh <build-dir> <version> <arch>   (arch: x86_64 | aarch64)
set -euo pipefail

# Resolve to an absolute path up front: the script cd's into the tools dir later,
# after which a relative build-dir argument would no longer resolve.
BUILD_DIR="$(realpath "$1")"
VERSION="$2"
ARCH="$3"

APPDIR="$BUILD_DIR/AppDir"
rm -rf "$APPDIR"

# Stage the install tree (binary + desktop + icon) into the AppDir.
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR" --prefix /usr

# Fetch linuxdeploy + the GTK plugin (cached across runs by the build dir).
TOOLS="$BUILD_DIR/appimage-tools"
mkdir -p "$TOOLS"
cd "$TOOLS"

LD="linuxdeploy-${ARCH}.AppImage"
[ -f "$LD" ] || curl -fsSL -o "$LD" \
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/${LD}"
[ -f linuxdeploy-plugin-gtk.sh ] || curl -fsSL -o linuxdeploy-plugin-gtk.sh \
  "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
chmod +x "$LD" linuxdeploy-plugin-gtk.sh

# No FUSE on CI runners — run the AppImages by extracting them instead.
export APPIMAGE_EXTRACT_AND_RUN=1
export DEPLOY_GTK_VERSION=4
export OUTPUT="copyclip-${VERSION}-${ARCH}.AppImage"

"./${LD}" --appdir "$APPDIR" --plugin gtk \
  --desktop-file "$APPDIR/usr/share/applications/dev.walkercito.CopyClip.desktop" \
  --icon-file "$APPDIR/usr/share/icons/hicolor/scalable/apps/dev.walkercito.CopyClip.svg" \
  --output appimage

mv "$OUTPUT" "$BUILD_DIR/${OUTPUT}"
echo "built ${BUILD_DIR}/${OUTPUT}"
