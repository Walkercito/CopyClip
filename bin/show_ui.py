#!/usr/bin/env python3
"""Helper script to show CopyClip UI manually if hotkeys don't work."""

import sys

from PyQt6.QtWidgets import QApplication

from copyclip.core.clipboard import ClipboardManager
from copyclip.core.history import HistoryManager
from copyclip.core.settings import SettingsManager
from copyclip.ui.main_window import MainWindow


def main():
    """Show the CopyClip UI."""
    app = QApplication(sys.argv)

    clipboard_manager = ClipboardManager()
    history_manager = HistoryManager(clipboard_manager)
    settings_manager = SettingsManager()

    main_window = MainWindow(history_manager, settings_manager)
    main_window.show_clipboard()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
