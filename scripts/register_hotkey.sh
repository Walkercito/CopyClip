#!/bin/bash
# Script to register CopyClip global hotkey in GNOME

set -e

HOTKEY="${1:-<Primary><Shift>v}"
CUSTOM_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/copyclip/"
SCHEMA="org.gnome.settings-daemon.plugins.media-keys"

echo "Registering CopyClip hotkey in GNOME..."
echo "Hotkey: $HOTKEY"
echo "Command: copyclip-show-ui"
echo ""

if ! command -v gsettings &> /dev/null; then
    echo "ERROR: gsettings not found. Are you running GNOME?"
    exit 1
fi

CURRENT=$(gsettings get $SCHEMA custom-keybindings)
echo "Current custom keybindings: $CURRENT"

if [[ "$CURRENT" == *"$CUSTOM_PATH"* ]]; then
    echo "CopyClip keybinding already exists, updating..."
else
    if [[ "$CURRENT" == "@as []" ]]; then
        NEW_LIST="['$CUSTOM_PATH']"
    else
        PATHS=$(echo "$CURRENT" | grep -oP "'/[^']+/'|/[^,\]]+/" | sed "s|^/|'/|;s|/$|/'|;s|^'//|'/|;s|//'$|/'|")

        NEW_LIST="["
        FIRST=true
        for path in $PATHS; do
            # Ensure path has quotes and trailing slash
            clean_path=$(echo "$path" | sed "s/'//g")
            if [[ "$clean_path" != */ ]]; then
                clean_path="${clean_path}/"
            fi

            if [ "$FIRST" = true ]; then
                NEW_LIST="${NEW_LIST}'${clean_path}'"
                FIRST=false
            else
                NEW_LIST="${NEW_LIST}, '${clean_path}'"
            fi
        done
        NEW_LIST="${NEW_LIST}, '$CUSTOM_PATH']"
    fi

    echo "Setting custom-keybindings to: $NEW_LIST"
    gsettings set $SCHEMA custom-keybindings "$NEW_LIST"
fi

echo "Setting keybinding properties..."
gsettings set $SCHEMA.custom-keybinding:$CUSTOM_PATH name 'CopyClip'
gsettings set $SCHEMA.custom-keybinding:$CUSTOM_PATH command 'copyclip-show-ui'
gsettings set $SCHEMA.custom-keybinding:$CUSTOM_PATH binding "$HOTKEY"

echo ""
echo "✓ Hotkey registered successfully!"
echo ""
echo "You can now press $HOTKEY to open CopyClip from anywhere."
echo ""
echo "Note: Make sure 'copyclip-show-ui' is in your PATH."
echo "Run this to add it (from project root):"
echo "  mkdir -p ~/.local/bin"
echo "  ln -sf \"\$PROJECT_ROOT/bin/copyclip-show-ui\" ~/.local/bin/copyclip-show-ui"
