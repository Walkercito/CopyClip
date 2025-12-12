"""Core functionality for clipboard and history management."""

from copyclip.core.clipboard import ClipboardManager
from copyclip.core.history import HistoryItem, HistoryManager
from copyclip.core.settings import SettingsManager

__all__ = [
    "ClipboardManager",
    "HistoryManager",
    "HistoryItem",
    "SettingsManager",
]
