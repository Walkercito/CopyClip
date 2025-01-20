# history_manager.py
import os
import json
from pathlib import Path
from datetime import datetime

class HistoryManager:
    """Manages the clipboard history and its persistent storage."""
    
    def __init__(self, clipboard_manager):
        """Initialize the history manager with a clipboard manager instance.
        
        Args:
            clipboard_manager: The clipboard manager instance to work with.
        """
        self.clipboard_manager = clipboard_manager
        self.data_dir = os.path.join(
            os.getenv('XDG_DATA_HOME', os.path.expanduser('~/.local/share')), 'clipboard-manager')
        Path(self.data_dir).mkdir(parents=True, exist_ok=True)
        
        self.history_file = os.path.join(self.data_dir, 'clipboard_history.json')
        self.history = self.load_history()

    def load_history(self):
        """Load clipboard history from the JSON file.
        
        Returns:
            list: The loaded history or an empty list if loading fails.
        """
        try:
            if os.path.exists(self.history_file):
                with open(self.history_file, 'r', encoding='utf-8') as file:
                    return json.load(file)
        except Exception as e:
            print(f"Error loading history: {e}")
        return []

    def save_history(self):
        """Save the current clipboard history to the JSON file."""
        try:
            with open(self.history_file, 'w', encoding='utf-8') as file:
                json.dump(self.history, file, ensure_ascii=False, indent=2)
        except Exception as e:
            print(f"Error saving history: {e}")

    def add_to_history(self, content):
        """Add a new item to the clipboard history.
        
        If the item already exists, it is moved to the top of the history.
        
        Args:
            content: The content to add to the history.
        """
        if content and content.strip():
            for item in self.history:
                if item['content'] == content:
                    self.history.remove(item)
                    self.history.insert(0, item)
                    self.save_history()
                    return
                    
            # Add new item
            new_item = {
                'content': content,
                'timestamp': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                'pinned': False
            }
            self.history.insert(0, new_item)
            self.save_history()

    def toggle_pin(self, content):
        """Toggle the pinned status of a history item.
        
        Args:
            content: The content of the item to toggle.
            
        Returns:
            bool: The new pinned status of the item.
        """
        for item in self.history:
            if item['content'] == content:
                item['pinned'] = not item['pinned']
                self.save_history()
                return item['pinned']
        return False

    def remove_item(self, content):
        """Remove an item from the history.
        
        Args:
            content: The content of the item to remove.
        """
        self.history = [item for item in self.history if item['content'] != content]
        self.save_history()

    def get_history(self):
        """Get the current clipboard history.
        
        Returns:
            list: The current clipboard history.
        """
        return self.history