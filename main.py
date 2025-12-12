"""CopyClip - Clipboard Manager for Linux."""

import signal
import sys

from PyQt6.QtWidgets import QApplication

from copyclip.core.clipboard import ClipboardManager
from copyclip.core.history import HistoryManager
from copyclip.core.settings import SettingsManager
from copyclip.hotkeys.manager import HotkeyManager
from copyclip.ui.dialogs import FirstRunDialog
from copyclip.ui.main_window import MainWindow


def main():
    """Main entry point for CopyClip."""
    app = QApplication(sys.argv)

    # Initialize managers
    clipboard_manager = ClipboardManager()
    history_manager = HistoryManager(clipboard_manager)
    settings_manager = SettingsManager()

    # Check if this is the first run
    if settings_manager.is_first_run():
        print("First run detected, showing hotkey selection dialog...")
        first_run_dialog = FirstRunDialog()
        result = first_run_dialog.exec()

        if result:
            selected_preset = first_run_dialog.get_selected_preset()
            print(f"User selected hotkey: {selected_preset}")
            settings_manager.set("hotkey", selected_preset)
            settings_manager.set("first_run_completed", True)
            settings_manager.save()
        else:
            # User cancelled, use default
            print("User cancelled, using default hotkey")
            settings_manager.set("hotkey", "super_v")
            settings_manager.set("first_run_completed", True)
            settings_manager.save()

    # Initialize main window
    main_window = MainWindow(history_manager, settings_manager)

    # Get the configured hotkey
    hotkey_preset = settings_manager.get("hotkey", "super_v")
    print(f"Initializing with hotkey preset: {hotkey_preset}")

    # Initialize hotkey manager with configured hotkey
    try:
        hotkey_manager = HotkeyManager(
            clipboard_manager, history_manager, main_window, hotkey_preset
        )
        main_window.set_hotkey_manager(hotkey_manager)  # Link for settings updates
    except Exception as e:
        print(f"Error initializing hotkey manager: {e}")
        print("The application will continue, but hotkeys may not work.")
        print("Please check if you're running under X11 (not Wayland).")
        hotkey_manager = None

    # Setup signal handlers for clean shutdown
    def signal_handler(signum, frame):
        print("\nShutting down CopyClip...")
        if hotkey_manager:
            hotkey_manager.stop_listening()
        app.quit()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    # Setup hotkeys if manager was initialized successfully
    if hotkey_manager:
        success = hotkey_manager.setup_hotkeys()
        if not success:
            print("Warning: Failed to setup hotkeys. The UI can still be opened manually.")
    else:
        print("Hotkey manager not initialized. The UI can still be opened manually.")

    # Start the application
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
