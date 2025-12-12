"""History manager for clipboard items."""

import json
from datetime import datetime
from typing import TypedDict

from copyclip.core.clipboard import ClipboardManager
from copyclip.utils.constants import DATA_DIR, HISTORY_FILE


class HistoryItem(TypedDict):
    """Represents a clipboard history item."""

    content: str
    timestamp: str
    pinned: bool


class HistoryManager:
    """Manages clipboard history and its persistent storage."""

    def __init__(self, clipboard_manager: ClipboardManager) -> None:
        """Initialize the history manager.

        Args:
            clipboard_manager: The clipboard manager instance to work with
        """
        self.clipboard_manager = clipboard_manager
        self.history_file = HISTORY_FILE

        # Ensure data directory exists
        DATA_DIR.mkdir(parents=True, exist_ok=True)

        self.history: list[HistoryItem] = self._load_history()

    def _load_history(self) -> list[HistoryItem]:
        """Load clipboard history from the JSON file.

        Returns:
            The loaded history or an empty list if loading fails
        """
        try:
            if self.history_file.exists():
                with open(self.history_file, encoding="utf-8") as file:
                    data = json.load(file)
                    return data if isinstance(data, list) else []
        except Exception as e:
            print(f"Error loading history: {e}")
        return []

    def _save_history(self) -> None:
        """Save the current clipboard history to the JSON file."""
        backup_file = self.history_file.with_suffix(".json.backup")

        try:
            # Create backup if file exists
            if self.history_file.exists():
                try:
                    self.history_file.replace(backup_file)
                except Exception as e:
                    print(f"Warning: Could not create backup: {e}")

            # Save history
            with open(self.history_file, "w", encoding="utf-8") as file:
                json.dump(self.history, file, ensure_ascii=False, indent=2)

        except Exception as e:
            print(f"Error saving history: {e}")
            # Try to restore from backup
            if backup_file.exists():
                try:
                    backup_file.replace(self.history_file)
                except Exception as restore_error:
                    print(f"Error restoring from backup: {restore_error}")

    def add_item(self, content: str) -> bool:
        """Add a new item to the clipboard history.

        If the item already exists, it is moved to the top of the history.

        Args:
            content: The content to add to the history

        Returns:
            True if item was added/moved, False otherwise
        """
        if not content or not content.strip():
            return False

        # Check if content already exists
        for item in self.history:
            if item["content"] == content:
                self.history.remove(item)
                self.history.insert(0, item)
                self._save_history()
                return True

        # Create new item
        new_item: HistoryItem = {
            "content": content,
            "timestamp": datetime.now().strftime("%d/%m/%y %H:%M"),
            "pinned": False,
        }
        self.history.insert(0, new_item)
        self._save_history()
        return True

    def toggle_pin(self, content: str) -> bool:
        """Toggle the pinned status of a history item.

        Args:
            content: The content of the item to toggle

        Returns:
            The new pinned status of the item, False if not found
        """
        for item in self.history:
            if item["content"] == content:
                item["pinned"] = not item["pinned"]
                self._save_history()
                return item["pinned"]
        return False

    def clear_unpinned(self) -> None:
        """Clear all unpinned items from history."""
        self.history = [item for item in self.history if item.get("pinned", False)]
        self._save_history()

    def remove_item(self, content: str) -> None:
        """Remove an item from the history.

        Args:
            content: The content of the item to remove
        """
        self.history = [item for item in self.history if item["content"] != content]
        self._save_history()

    def get_sorted_history(self) -> list[HistoryItem]:
        """Get history with pinned items first, then unpinned in chronological order.

        Returns:
            Sorted history with pinned items first
        """
        pinned = [item for item in self.history if item.get("pinned", False)]
        unpinned = [item for item in self.history if not item.get("pinned", False)]

        pinned.sort(key=lambda x: x["timestamp"], reverse=True)
        unpinned.sort(key=lambda x: x["timestamp"], reverse=True)

        return pinned + unpinned

    def get_history(self) -> list[HistoryItem]:
        """Get the current clipboard history with pinned items first.

        Returns:
            The current clipboard history, sorted with pinned items first
        """
        return self.get_sorted_history()

    def get_pinned_items(self) -> list[HistoryItem]:
        """Get all pinned items from history.

        Returns:
            List of pinned history items
        """
        return [item for item in self.history if item["pinned"]]
