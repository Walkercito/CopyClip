"""Single instance manager to prevent multiple app instances."""

import os

from copyclip.utils.constants import DATA_DIR


class SingleInstance:
    """Manages single instance lock for the application."""

    def __init__(self):
        """Initialize the single instance manager."""
        self.lock_file = DATA_DIR / ".lock"
        self.signal_file = DATA_DIR / ".show_signal"
        self.pid = os.getpid()

        DATA_DIR.mkdir(parents=True, exist_ok=True)

    def is_running(self) -> bool:
        """Check if another instance is already running.

        Returns:
            True if another instance is running, False otherwise
        """
        if not self.lock_file.exists():
            return False

        try:
            # Read PID from lock file
            pid = int(self.lock_file.read_text().strip())

            # Check if process with that PID exists
            try:
                os.kill(pid, 0)  # Signal 0 doesn't kill, just checks existence
                return True  # Process exists
            except OSError:
                # Process doesn't exist, stale lock file
                self.lock_file.unlink()
                return False
        except (OSError, ValueError):
            # Invalid lock file, remove it
            with open(self.lock_file, "w"):
                pass
            self.lock_file.unlink()
            return False

    def acquire_lock(self) -> bool:
        """Acquire the instance lock.

        Returns:
            True if lock was acquired, False if another instance is running
        """
        if self.is_running():
            return False

        # Write our PID to lock file
        self.lock_file.write_text(str(self.pid))
        return True

    def release_lock(self) -> None:
        """Release the instance lock."""
        if self.lock_file.exists():
            try:
                # Only remove if it's our lock
                pid = int(self.lock_file.read_text().strip())
                if pid == self.pid:
                    self.lock_file.unlink()
            except (OSError, ValueError):
                pass

    def signal_show_window(self) -> bool:
        """Signal the running instance to show its window.

        Returns:
            True if signal was sent, False if no instance is running
        """
        if not self.is_running():
            return False

        # Create signal file
        self.signal_file.touch()
        return True

    def check_show_signal(self) -> bool:
        """Check if there's a signal to show the window.

        Returns:
            True if signal exists, False otherwise
        """
        if self.signal_file.exists():
            try:
                self.signal_file.unlink()
                return True
            except OSError:
                pass
        return False
