# Packaging: install the GTK app + desktop entry + icon, and configure CPack to
# build lightweight .deb / .rpm / .tar.gz that carry only the runtime files
# (stripped binary, no headers, tests, or debug symbols). Runs after src/ so the
# copyclip-gtk target already exists.

include(GNUInstallDirs)

# The product binary ships as "copyclip" (the GTK app is the shipped app); the
# build output keeps its copyclip-gtk name so existing scripts/CI are unaffected.
install(
  PROGRAMS "$<TARGET_FILE:copyclip-gtk>"
  DESTINATION "${CMAKE_INSTALL_BINDIR}"
  RENAME copyclip)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/dev.walkercito.CopyClip.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/dev.walkercito.CopyClip.svg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")

# --- CPack --------------------------------------------------------------------
# Version defaults to the project version; the release CI overrides it from the
# git tag with -DCOPYCLIP_VERSION=<x.y.z>.
set(COPYCLIP_VERSION "${PROJECT_VERSION}" CACHE STRING "Release version for packages")

set(CPACK_PACKAGE_NAME "copyclip")
set(CPACK_PACKAGE_VERSION "${COPYCLIP_VERSION}")
set(CPACK_PACKAGE_VENDOR "Walkercito")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Clipboard history manager for Linux (GTK4/libadwaita)")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Walkercito/CopyClip")
set(CPACK_PACKAGE_CONTACT "Walkercito <walkercitoliver@gmail.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_RELOCATABLE OFF)

# Base name used by the archive generators; .deb/.rpm override it below with their
# own conventions (copyclip-<ver>-<arch>.tar.gz).
set(CPACK_PACKAGE_FILE_NAME "copyclip-${COPYCLIP_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

# Refresh the desktop and icon caches so the app appears in the menu immediately.
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst;${CMAKE_CURRENT_SOURCE_DIR}/scripts/postrm")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/postrm")

# .deb: let dpkg-shlibdeps compute the exact shared-library dependencies from the
# binary; recommend the gsettings CLI (the global shortcut) and an input tool
# (auto-paste), which the app degrades gracefully without.
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libglib2.0-bin")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "wtype, ydotool, xdotool")

# .rpm: let rpmbuild auto-require the shared libraries.
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_PACKAGE_SUGGESTS "wtype")

include(CPack)
