"""Wayland hotkey backend using D-Bus."""

import subprocess
from typing import TYPE_CHECKING

from PyQt6.QtCore import QObject, pyqtSignal

from copyclip.hotkeys.backend_base import HotkeyBackend

if TYPE_CHECKING:
    from copyclip.core.clipboard import ClipboardManager
    from copyclip.core.history import HistoryManager
    from copyclip.hotkeys.config import HotkeyBinding
    from copyclip.ui.main_window import MainWindow


class WaylandHotkeySignals(QObject):
    """Qt signals for Wayland hotkey events."""

    show_clipboard_signal = pyqtSignal()
    content_copied_signal = pyqtSignal(str)


class WaylandHotkeyBackend(HotkeyBackend):
    """Wayland hotkey backend using D-Bus to register global shortcuts.

    This backend works with GNOME Shell and other compositors that support
    the org.gnome.Shell interface for global shortcuts.
    """

    def __init__(
        self,
        clipboard_manager: "ClipboardManager",
        history_manager: "HistoryManager",
        ui_manager: "MainWindow",
        binding: "HotkeyBinding",
    ):
        """Initialize the Wayland hotkey backend.

        Args:
            clipboard_manager: ClipboardManager instance
            history_manager: HistoryManager instance
            ui_manager: Main window instance
            binding: Hotkey binding configuration
        """
        self.clipboard_manager = clipboard_manager
        self.history_manager = history_manager
        self.ui_manager = ui_manager
        self.binding = binding
        self.debug = False

        # Setup Qt signals
        self.signals = WaylandHotkeySignals()
        self.signals.show_clipboard_signal.connect(self.ui_manager.show_clipboard)
        self.signals.content_copied_signal.connect(self._handle_copied_content)

        # D-Bus interface
        self.dbus_interface = None
        self.shortcut_name = "copyclip-show-clipboard"

    def _handle_copied_content(self, content: str) -> None:
        """Handle content that was copied to clipboard.

        Args:
            content: The copied content
        """
        if content:
            self.history_manager.add_item(content)
            self.clipboard_manager.set_content(content)

    def _convert_binding_to_accelerator(self) -> str:
        """Convert HotkeyBinding to GTK accelerator format.

        Returns:
            Accelerator string (e.g., '<Primary><Shift>v')
        """
        # Map modifier names to GTK accelerator format
        modifier_map = {
            "ctrl": "<Primary>",
            "alt": "<Alt>",
            "super": "<Super>",
            "shift": "<Shift>",
        }

        modifiers = self.binding.modifier.split("+")
        accelerator_parts = []

        for mod in modifiers:
            if mod in modifier_map:
                accelerator_parts.append(modifier_map[mod])

        # Add the key
        accelerator_parts.append(self.binding.key.lower())

        return "".join(accelerator_parts)

    def _register_via_gsettings(self) -> bool:  # noqa: PLR0912
        """Register hotkey via gsettings (GNOME).

        Returns:
            True if registration was successful, False otherwise
        """
        try:
            accelerator = self._convert_binding_to_accelerator()

            if self.debug:
                print(f"Registering hotkey via gsettings: {accelerator}")

            # Try to use gsettings to add custom keybinding
            # This creates a custom keybinding in GNOME
            schema = "org.gnome.settings-daemon.plugins.media-keys"

            # Get list of custom keybindings
            result = subprocess.run(
                ["gsettings", "get", schema, "custom-keybindings"],
                capture_output=True,
                text=True,
                check=False,
            )

            if result.returncode != 0:
                if self.debug:
                    print("gsettings not available or schema not found")
                return False

            custom_path = (
                "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/copyclip/"
            )

            # Set the custom keybinding
            commands = [
                [
                    "gsettings",
                    "set",
                    f"{schema}.custom-keybinding:{custom_path}",
                    "name",
                    "CopyClip",
                ],
                [
                    "gsettings",
                    "set",
                    f"{schema}.custom-keybinding:{custom_path}",
                    "command",
                    "copyclip-show-ui",
                ],
                [
                    "gsettings",
                    "set",
                    f"{schema}.custom-keybinding:{custom_path}",
                    "binding",
                    accelerator,
                ],
            ]

            for cmd in commands:
                result = subprocess.run(cmd, capture_output=True, check=False)
                if result.returncode != 0:
                    if self.debug:
                        print(f"Failed to run: {' '.join(cmd)}")
                    return False

            current_bindings = result.stdout.strip()
            if current_bindings == "@as []":
                new_bindings = f"['{custom_path}']"
            else:
                current_bindings = current_bindings.strip("[]").replace("'", "")
                if current_bindings:
                    bindings_list = [b.strip() for b in current_bindings.split(",") if b.strip()]
                    if custom_path not in bindings_list:
                        bindings_list.append(custom_path)
                    new_bindings = "[" + ", ".join(f"'{b}'" for b in bindings_list) + "]"
                else:
                    new_bindings = f"['{custom_path}']"

            result = subprocess.run(
                ["gsettings", "set", schema, "custom-keybindings", new_bindings],
                capture_output=True,
                check=False,
            )

            if result.returncode == 0:
                print(f"✓ Hotkey registered via GNOME settings: {accelerator}")
                print("  Note: The system will execute 'copyclip-show-ui' command")
                return True

            return False

        except Exception as e:
            if self.debug:
                print(f"Error registering via gsettings: {e}")
            return False

    def setup(self) -> bool:
        """Setup hotkey monitoring via D-Bus.

        Returns:
            True if setup was successful, False otherwise
        """
        print("\n" + "=" * 70)
        print("  Wayland Session Detected")
        print("=" * 70)
        print("\nGlobal hotkeys are not fully supported on Wayland yet.")
        print("You can configure the hotkey manually in your system settings:\n")
        print("  Name:    CopyClip")
        print(f"  Hotkey:  {self.binding.display_name}")
        print("  Command: copyclip-show-ui\n")
        print("Instructions:")
        print("  GNOME: Settings → Keyboard → View and Customize Shortcuts → Custom Shortcuts")
        print("  KDE:   System Settings → Shortcuts → Custom Shortcuts\n")
        print("Alternatively, you can run CopyClip manually:")
        print("  - Execute 'copyclip-show-ui' from terminal")
        print("  - Or run 'uv run show_ui.py' from the project directory")
        print("=" * 70 + "\n")

        return False

    def stop(self) -> None:
        """Stop hotkey monitoring and cleanup."""
        pass

    def update_hotkey(self, binding: "HotkeyBinding") -> bool:
        """Update the hotkey binding.

        Args:
            binding: New hotkey binding

        Returns:
            True if update was successful, False otherwise
        """
        self.binding = binding
        # Re-register with new binding
        return self.setup()

    def enable_debug(self) -> None:
        """Enable debug logging."""
        self.debug = True
        print("Debug logging enabled (Wayland backend)")
