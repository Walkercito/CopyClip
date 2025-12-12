#!/bin/bash
# Remove CopyClip hotkey from GNOME

set -e

CUSTOM_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/copyclip/"
SCHEMA="org.gnome.settings-daemon.plugins.media-keys"

echo "Removing CopyClip hotkey from GNOME..."

CURRENT=$(gsettings get $SCHEMA custom-keybindings)
echo "Current custom keybindings: $CURRENT"

if [[ "$CURRENT" == *"$CUSTOM_PATH"* ]]; then
    NEW_LIST=$(echo "$CURRENT" | sed "s|, '$CUSTOM_PATH'||;s|'$CUSTOM_PATH', ||;s|'$CUSTOM_PATH'||")

    if [[ "$NEW_LIST" == "[]" ]]; then
        NEW_LIST="@as []"
    fi

    echo "Removing CopyClip keybinding..."
    gsettings set $SCHEMA custom-keybindings "$NEW_LIST"

    gsettings reset $SCHEMA.custom-keybinding:$CUSTOM_PATH name 2>/dev/null || true
    gsettings reset $SCHEMA.custom-keybinding:$CUSTOM_PATH command 2>/dev/null || true
    gsettings reset $SCHEMA.custom-keybinding:$CUSTOM_PATH binding 2>/dev/null || true

    echo "✓ CopyClip hotkey removed"
else
    echo "CopyClip keybinding not found"
fi

echo ""
echo "You can now use Ctrl+Shift+V normally again"
