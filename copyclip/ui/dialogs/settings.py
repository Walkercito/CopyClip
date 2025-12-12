"""Settings dialog for application configuration."""

from collections.abc import Callable
from typing import TYPE_CHECKING

from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
)

from copyclip.hotkeys.config import HotkeyConfig
from copyclip.utils.constants import Theme

if TYPE_CHECKING:
    from copyclip.core.settings import SettingsManager


class SettingsDialog(QDialog):
    """Dialog for changing application settings."""

    def __init__(
        self,
        settings_manager: "SettingsManager",
        on_theme_changed: Callable[[str], None],
        on_hotkey_changed: Callable[[str], None],
        parent=None,
    ):
        """Initialize the settings dialog.

        Args:
            settings_manager: Settings manager instance
            on_theme_changed: Callback for theme changes
            on_hotkey_changed: Callback for hotkey changes
            parent: Parent widget
        """
        super().__init__(parent)
        self.settings_manager = settings_manager
        self.on_theme_changed = on_theme_changed
        self.on_hotkey_changed = on_hotkey_changed
        self._setup_ui()

    def _setup_ui(self) -> None:
        """Setup the dialog UI."""
        self.setWindowTitle("Settings")
        self.setMinimumWidth(350)
        self.setModal(True)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(15, 15, 15, 15)
        layout.setSpacing(10)

        # Theme setting
        self._create_theme_section(layout)

        # Hotkey setting
        self._create_hotkey_section(layout)

        layout.addStretch(1)

        # Close button
        button_box = QHBoxLayout()
        button_box.addStretch(1)
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.accept)
        button_box.addWidget(close_btn)
        layout.addLayout(button_box)

        # Position dialog relative to parent
        self._position_dialog()

    def _create_theme_section(self, layout: QVBoxLayout) -> None:
        """Create the theme selection section.

        Args:
            layout: Parent layout
        """
        theme_label = QLabel("Theme:")
        layout.addWidget(theme_label)

        theme_combo = QComboBox()
        theme_combo.setObjectName("themeComboBox")
        theme_combo.addItems([Theme.DARK, Theme.LIGHT, Theme.SYSTEM])

        current_theme = self.settings_manager.get("theme", Theme.DARK)
        theme_combo.setCurrentText(current_theme)
        theme_combo.currentTextChanged.connect(self._handle_theme_change)
        layout.addWidget(theme_combo)

    def _create_hotkey_section(self, layout: QVBoxLayout) -> None:
        """Create the hotkey selection section.

        Args:
            layout: Parent layout
        """
        hotkey_label = QLabel("Hotkey to open clipboard:")
        layout.addWidget(hotkey_label)

        hotkey_combo = QComboBox()
        hotkey_combo.setObjectName("hotkeyComboBox")

        presets = HotkeyConfig.get_all_presets()
        for preset_name, binding in presets.items():
            hotkey_combo.addItem(binding.display_name, preset_name)

        current_hotkey = self.settings_manager.get("hotkey", "super_v")
        for i in range(hotkey_combo.count()):
            if hotkey_combo.itemData(i) == current_hotkey:
                hotkey_combo.setCurrentIndex(i)
                break

        hotkey_combo.currentIndexChanged.connect(
            lambda index: self._handle_hotkey_change(hotkey_combo, index)
        )
        layout.addWidget(hotkey_combo)

        hotkey_note = QLabel(
            "Note: Changes apply immediately. Restart may be required if hotkey doesn't work."
        )
        hotkey_note.setWordWrap(True)
        hotkey_note_font = QFont()
        hotkey_note_font.setPointSize(8)
        hotkey_note_font.setItalic(True)
        hotkey_note.setFont(hotkey_note_font)
        layout.addWidget(hotkey_note)

    def _handle_theme_change(self, theme: str) -> None:
        """Handle theme change.

        Args:
            theme: New theme name
        """
        self.settings_manager.set("theme", theme)
        self.settings_manager.save()
        self.on_theme_changed(theme)

    def _handle_hotkey_change(self, combo: QComboBox, index: int) -> None:
        """Handle hotkey change.

        Args:
            combo: Hotkey combo box
            index: Selected index
        """
        new_preset = combo.itemData(index)
        self.settings_manager.set("hotkey", new_preset)
        self.settings_manager.save()
        self.on_hotkey_changed(new_preset)

    def _position_dialog(self) -> None:
        """Position dialog relative to parent window."""
        try:
            parent = self.parent()
            if parent:
                main_geo = parent.geometry()
                dialog_size = self.sizeHint()
                x = main_geo.x() + (main_geo.width() - dialog_size.width()) // 2
                y = main_geo.y() + (main_geo.height() - dialog_size.height()) // 2
                self.move(max(0, x), max(0, y))
        except Exception as e:
            print(f"Could not position settings dialog: {e}")
