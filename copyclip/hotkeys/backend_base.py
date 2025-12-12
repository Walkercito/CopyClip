"""Base class for hotkey backends."""

from abc import ABC, abstractmethod
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from copyclip.hotkeys.config import HotkeyBinding


class HotkeyBackend(ABC):
    """Abstract base class for hotkey backends."""

    @abstractmethod
    def setup(self) -> bool:
        """Setup the hotkey monitoring.

        Returns:
            True if setup was successful, False otherwise
        """
        pass

    @abstractmethod
    def stop(self) -> None:
        """Stop hotkey monitoring and cleanup resources."""
        pass

    @abstractmethod
    def update_hotkey(self, binding: "HotkeyBinding") -> bool:
        """Update the hotkey binding.

        Args:
            binding: New hotkey binding

        Returns:
            True if update was successful, False otherwise
        """
        pass

    @abstractmethod
    def enable_debug(self) -> None:
        """Enable debug logging."""
        pass
