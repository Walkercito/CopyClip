"""Clipboard manager using xsel for X11."""

import subprocess


class ClipboardManager:
    """Manages clipboard operations using xsel."""

    def __init__(self) -> None:
        """Initialize the clipboard manager using xsel.

        Raises:
            RuntimeError: If xsel is not installed on the system
        """
        self._verify_xsel_installed()
        self.current_clipboard: str | None = self.get_content()

    def _verify_xsel_installed(self) -> None:
        """Verify that xsel is installed on the system.

        Raises:
            RuntimeError: If xsel is not found
        """
        try:
            subprocess.run(["which", "xsel"], check=True, capture_output=True)
        except subprocess.CalledProcessError as err:
            msg = (
                "xsel is not installed. Please install it:\n"
                "  Ubuntu/Debian: sudo apt-get install xsel\n"
                "  Fedora: sudo dnf install xsel\n"
                "  Arch: sudo pacman -S xsel"
            )
            raise RuntimeError(msg) from err

    def get_content(self) -> str | None:
        """Get the current content of the clipboard.

        Returns:
            The clipboard content as a string, or None if there was an error
        """
        try:
            result = subprocess.run(
                ["xsel", "-b", "-o"], check=False, capture_output=True, text=True
            )
            return result.stdout.strip() if result.stdout else None
        except Exception as e:
            print(f"Error getting clipboard content: {e}")
            return None

    def set_content(self, content: str) -> bool:
        """Set new clipboard content.

        Args:
            content: The text content to set in the clipboard

        Returns:
            True if successful, False otherwise
        """
        try:
            process = subprocess.Popen(["xsel", "-b", "-i"], stdin=subprocess.PIPE)
            process.communicate(input=content.encode())
            self.current_clipboard = content
            return True
        except Exception as e:
            print(f"Error setting clipboard: {e}")
            return False

    def check_for_changes(self) -> str | None:
        """Check if the clipboard content has changed.

        Returns:
            The new content if it changed, None otherwise
        """
        try:
            content = self.get_content()
            if content is not None and content != self.current_clipboard:
                self.current_clipboard = content
                return content
        except Exception as e:
            print(f"Error checking clipboard content: {e}")
        return None
