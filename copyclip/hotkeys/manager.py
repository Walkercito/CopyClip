"""Hotkey manager that auto-detects and uses appropriate backend (X11 or Wayland)."""

from typing import TYPE_CHECKING

from copyclip.hotkeys.backend_base import HotkeyBackend
from copyclip.hotkeys.config import HotkeyConfig
from copyclip.utils.environment import get_session_type

if TYPE_CHECKING:
    from copyclip.core.clipboard import ClipboardManager
    from copyclip.core.history import HistoryManager
    from copyclip.ui.main_window import MainWindow


class HotkeyManager:
    """Manages global hotkeys using the appropriate backend for the current environment."""

    def __init__(
        self,
        clipboard_manager: "ClipboardManager",
        history_manager: "HistoryManager",
        ui_manager: "MainWindow",
        hotkey_preset: str = "super_v",
    ) -> None:
        """Initialize the hotkey manager.

        Args:
            clipboard_manager: ClipboardManager instance
            history_manager: HistoryManager instance
            ui_manager: Main window instance
            hotkey_preset: Name of the hotkey preset to use (default: 'super_v')
        """
        self.clipboard_manager = clipboard_manager
        self.history_manager = history_manager
        self.ui_manager = ui_manager

        # Load hotkey configuration
        self.hotkey_binding = HotkeyConfig.get_binding(hotkey_preset)
        if not self.hotkey_binding:
            print(f"Warning: Invalid hotkey preset '{hotkey_preset}', using default")
            self.hotkey_binding = HotkeyConfig.get_default_binding()

        print(f"Using hotkey: {self.hotkey_binding.display_name}")

        # Detect session type and create appropriate backend
        session_type = get_session_type()
        print(f"Detected session type: {session_type}")

        self.backend: HotkeyBackend | None = None

        if session_type == "x11":
            print("Using X11 backend for global hotkeys")
            from copyclip.hotkeys.backend_x11 import X11HotkeyBackend  # noqa: PLC0415

            self.backend = X11HotkeyBackend(
                clipboard_manager, history_manager, ui_manager, self.hotkey_binding
            )
        elif session_type == "wayland":
            print("Using Wayland backend for global hotkeys")
            from copyclip.hotkeys.backend_wayland import WaylandHotkeyBackend  # noqa: PLC0415

            self.backend = WaylandHotkeyBackend(
                clipboard_manager, history_manager, ui_manager, self.hotkey_binding
            )
        else:
            print(f"Warning: Unknown session type '{session_type}'. Global hotkeys may not work.")
            print("Trying X11 backend as fallback...")
            try:
                from copyclip.hotkeys.backend_x11 import X11HotkeyBackend  # noqa: PLC0415

                self.backend = X11HotkeyBackend(
                    clipboard_manager, history_manager, ui_manager, self.hotkey_binding
                )
            except Exception as e:
                print(f"Failed to initialize fallback X11 backend: {e}")
                self.backend = None

    def setup_hotkeys(self) -> bool:
        """Setup keyboard monitoring.

        Returns:
            True if setup was successful, False otherwise
        """
        if not self.backend:
            print("No hotkey backend available")
            return False

        return self.backend.setup()

    def stop_listening(self) -> None:
        """Stop keyboard monitoring."""
        if self.backend:
            self.backend.stop()

    def update_hotkey(self, preset_name: str) -> bool:
        """Update the hotkey binding.

        Args:
            preset_name: Name of the new hotkey preset

        Returns:
            True if update was successful, False otherwise
        """
        new_binding = HotkeyConfig.get_binding(preset_name)
        if not new_binding:
            print(f"Error: Invalid hotkey preset '{preset_name}'")
            return False

        self.hotkey_binding = new_binding

        if self.backend:
            return self.backend.update_hotkey(new_binding)

        return False

    def enable_debug(self) -> None:
        """Enable debug logging."""
        if self.backend:
            self.backend.enable_debug()
