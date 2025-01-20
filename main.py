# main.py
import signal
from code.clipboard import ClipboardManager
from code.history_manager import HistoryManager
from code.ui import UIManager
from code.hotkeys import HotkeyManager


def main():
    clipboard_manager = ClipboardManager()
    history_manager = HistoryManager(clipboard_manager)
    ui_manager = UIManager(history_manager)
    hotkey_manager = HotkeyManager(clipboard_manager, history_manager, ui_manager)
    
    def signal_handler(signum, frame):
        print("\nCerrando aplicación...")
        hotkey_manager.stop_listening()
        ui_manager.root.quit()
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    hotkey_manager.setup_hotkeys()
    ui_manager.root.mainloop()

if __name__ == "__main__":
    main()