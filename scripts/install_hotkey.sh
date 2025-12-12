#!/bin/bash
# One-step installation script for CopyClip hotkey on Wayland/GNOME

set -e

echo "═══════════════════════════════════════════════════"
echo "  CopyClip Hotkey Installation for GNOME/Wayland"
echo "═══════════════════════════════════════════════════"
echo ""

SETTINGS_FILE="$HOME/.local/share/clipboard-manager/settings.json"
HOTKEY_PRESET="super_v"  # default

if [ -f "$SETTINGS_FILE" ]; then
    HOTKEY_PRESET=$(python3 -c "
import json
try:
    with open('$SETTINGS_FILE', 'r') as f:
        settings = json.load(f)
        print(settings.get('hotkey', 'super_v'))
except:
    print('super_v')
" 2>/dev/null || echo "super_v")
fi

case "$HOTKEY_PRESET" in
    "super_v")
        GTK_HOTKEY="<Super>v"
        ;;
    "ctrl_alt_v")
        GTK_HOTKEY="<Primary><Alt>v"
        ;;
    "super_c")
        GTK_HOTKEY="<Super>c"
        ;;
    "ctrl_shift_v")
        GTK_HOTKEY="<Primary><Shift>v"
        ;;
    *)
        GTK_HOTKEY="<Primary><Shift>v"
        ;;
esac

echo "Detected hotkey preset: $HOTKEY_PRESET"
echo "GTK accelerator: $GTK_HOTKEY"
echo ""

echo "[1/2] Installing copyclip-show-ui to ~/.local/bin..."

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

mkdir -p ~/.local/bin
ln -sf "$PROJECT_ROOT/bin/copyclip-show-ui" ~/.local/bin/copyclip-show-ui
chmod +x "$PROJECT_ROOT/bin/copyclip-show-ui"
echo "✓ Installed copyclip-show-ui"
echo ""

echo "[2/2] Registering global hotkey..."
./register_hotkey.sh "$GTK_HOTKEY"

echo ""
echo "═══════════════════════════════════════════════════"
echo "  Installation Complete!"
echo "═══════════════════════════════════════════════════"
echo ""
echo "Press $GTK_HOTKEY from anywhere to open CopyClip"
echo ""
echo "If the hotkey doesn't work immediately, try:"
echo "  1. Logging out and back in (to refresh PATH)"
echo "  2. Running: source ~/.bashrc (or ~/.zshrc)"
echo ""
