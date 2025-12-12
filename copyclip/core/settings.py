"""Settings manager for application configuration."""

import json
from datetime import datetime
from typing import Any

from copyclip.utils.constants import DATA_DIR, DEFAULT_SETTINGS, SETTINGS_FILE


class SettingsManager:
    """Manages application settings and persistence."""

    def __init__(self) -> None:
        """Initialize the settings manager."""
        self.settings_file = SETTINGS_FILE

        # Ensure data directory exists
        DATA_DIR.mkdir(parents=True, exist_ok=True)

        self.settings: dict[str, Any] = self._load_settings()

    def _load_settings(self) -> dict[str, Any]:
        """Load settings from the JSON file.

        Returns:
            The loaded settings or default settings if loading fails
        """
        try:
            if self.settings_file.exists():
                with open(self.settings_file, encoding="utf-8") as f:
                    loaded_settings = json.load(f)
                    return DEFAULT_SETTINGS | loaded_settings
            print("Settings file not found, using defaults.")
            return DEFAULT_SETTINGS.copy()

        except json.JSONDecodeError:
            print(f"Error: Settings file ({self.settings_file}) is corrupted.")
            self._backup_corrupted_file()
            return DEFAULT_SETTINGS.copy()

        except Exception as e:
            print(f"Error loading settings from {self.settings_file}: {e}")
            return DEFAULT_SETTINGS.copy()

    def _backup_corrupted_file(self) -> None:
        """Backup a corrupted settings file."""
        try:
            timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
            corrupted_path = self.settings_file.with_suffix(f".corrupted_{timestamp}.json")
            self.settings_file.rename(corrupted_path)
            print(f"Corrupted settings file backed up to {corrupted_path}")
        except OSError as e:
            print(f"Could not back up corrupted settings file: {e}")

    def save(self) -> bool:
        """Save current settings to the JSON file.

        Returns:
            True if save was successful, False otherwise
        """
        try:
            with open(self.settings_file, "w", encoding="utf-8") as f:
                json.dump(self.settings, f, indent=4)
            return True
        except Exception as e:
            print(f"Error saving settings to {self.settings_file}: {e}")
            return False

    def get(self, key: str, default: Any = None) -> Any:
        """Get a setting value.

        Args:
            key: The setting key
            default: Default value if key doesn't exist

        Returns:
            The setting value or default
        """
        return self.settings.get(key, default)

    def set(self, key: str, value: Any) -> None:
        """Set a setting value.

        Args:
            key: The setting key
            value: The value to set
        """
        self.settings[key] = value

    def is_first_run(self) -> bool:
        """Check if this is the first time the application is run.

        Returns:
            True if this is the first run, False otherwise
        """
        if not self.settings_file.exists():
            return True
        return not self.settings.get("first_run_completed", False)

    def mark_first_run_completed(self) -> None:
        """Mark that the first run has been completed."""
        self.settings["first_run_completed"] = True
        self.save()
