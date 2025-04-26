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
    
    def get_pinned_items(self):
        """Get all pinned items from history."""
        return [item for item in self.history if item['pinned']]



    def save_history(self):
        """Save the current clipboard history to the JSON file."""
        try:
            os.makedirs(os.path.dirname(self.history_file), exist_ok=True)
            
            if os.path.exists(self.history_file):
                backup_file = f"{self.history_file}.backup"
                try:
                    os.replace(self.history_file, backup_file)
                except Exception as e:
                    print(f"Warning: Could not create backup: {e}")
            
            with open(self.history_file, 'w', encoding='utf-8') as file:
                json.dump(self.history, file, ensure_ascii=False, indent=2)
                
        except Exception as e:
            print(f"Error saving history: {e}")
            if os.path.exists(backup_file):
                try:
                    os.replace(backup_file, self.history_file)
                except Exception as restore_error:
                    print(f"Error restoring from backup: {restore_error}")

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
                    
            new_item = {
                'content': content,
                'timestamp': datetime.now().strftime("%d/%m/%y %H:%M"),
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
    
    def clear_history(self):
        """Clear history while preserving pinned items.
        
        This method will:
        1. Get all pinned items
        2. Keep only pinned items in history
        3. Save the updated history
        """
        pinned_items = [item for item in self.history if item.get('pinned', False)]
        self.history = pinned_items
        self.save_history()

    def remove_item(self, content):
        """Remove an item from the history.
        
        Args:
            content: The content of the item to remove.
        """
        self.history = [item for item in self.history if item['content'] != content]
        self.save_history()

    def get_sorted_history(self):
        """Get history with pinned items first, then unpinned items in chronological order.
        
        Returns:
            list: Sorted history with pinned items first
        """
        pinned = [item for item in self.history if item.get('pinned', False)]
        unpinned = [item for item in self.history if not item.get('pinned', False)]
        
        pinned.sort(key=lambda x: x['timestamp'], reverse=True)
        unpinned.sort(key=lambda x: x['timestamp'], reverse=True)
        
        return pinned + unpinned

    def get_history(self):
        """Get the current clipboard history with pinned items first.
        
        Returns:
            list: The current clipboard history, sorted with pinned items first
        """
        return self.get_sorted_history()