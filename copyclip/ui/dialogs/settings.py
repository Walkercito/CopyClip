"""Settings dialog for application configuration."""

from collections.abc import Callable
from typing import TYPE_CHECKING

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDialog,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QVBoxLayout,
)

from copyclip.hotkeys.config import HotkeyConfig
from copyclip.utils.constants import APP_NAME, APP_VERSION, Theme

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
        self.setMinimumWidth(400)
        self.setModal(True)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 20, 20, 20)
        layout.setSpacing(15)

        # Header with app name and version
        self._create_header(layout)

        # Theme setting
        self._create_theme_section(layout)

        # Hotkey setting
        self._create_hotkey_section(layout)

        # Behavior settings
        self._create_behavior_section(layout)

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

    def _create_header(self, layout: QVBoxLayout) -> None:
        """Create the header with app name and version.

        Args:
            layout: Parent layout
        """
        header_label = QLabel(f"{APP_NAME} Settings")
        header_font = QFont()
        header_font.setPointSize(14)
        header_font.setBold(True)
        header_label.setFont(header_font)
        layout.addWidget(header_label)

        version_label = QLabel(f"Version {APP_VERSION}")
        version_label.setObjectName("subtitleLabel")
        layout.addWidget(version_label)

    def _create_theme_section(self, layout: QVBoxLayout) -> None:
        """Create the theme selection section.

        Args:
            layout: Parent layout
        """
        theme_label = QLabel("Theme:")
        theme_label_font = QFont()
        theme_label_font.setBold(True)
        theme_label.setFont(theme_label_font)
        layout.addWidget(theme_label)

        theme_combo = QComboBox()
        theme_combo.setObjectName("themeComboBox")
        theme_combo.addItems([Theme.DARK, Theme.LIGHT, Theme.SYSTEM])

        current_theme = self.settings_manager.get("theme", Theme.DARK)
        theme_combo.setCurrentText(current_theme)
        theme_combo.currentTextChanged.connect(self._handle_theme_change)
        layout.addWidget(theme_combo)

        # Description
        theme_desc = QLabel("Choose the color scheme for the application")
        theme_desc.setObjectName("noteLabel")
        layout.addWidget(theme_desc)

    def _create_hotkey_section(self, layout: QVBoxLayout) -> None:
        """Create the hotkey selection section.

        Args:
            layout: Parent layout
        """
        hotkey_label = QLabel("Hotkey:")
        hotkey_label_font = QFont()
        hotkey_label_font.setBold(True)
        hotkey_label.setFont(hotkey_label_font)
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

        # Description
        hotkey_note = QLabel("Keyboard shortcut to open the clipboard manager")
        hotkey_note.setObjectName("noteLabel")
        layout.addWidget(hotkey_note)

    def _create_behavior_section(self, layout: QVBoxLayout) -> None:
        """Create the behavior settings section.

        Args:
            layout: Parent layout
        """
        behavior_label = QLabel("Behavior:")
        behavior_label_font = QFont()
        behavior_label_font.setBold(True)
        behavior_label.setFont(behavior_label_font)
        layout.addWidget(behavior_label)

        # Auto-hide option
        self.auto_hide_checkbox = QCheckBox("Auto-hide window after copying")
        auto_hide_enabled = self.settings_manager.get("auto_hide_on_copy", True)
        self.auto_hide_checkbox.setChecked(auto_hide_enabled)
        self.auto_hide_checkbox.stateChanged.connect(self._handle_auto_hide_change)
        layout.addWidget(self.auto_hide_checkbox)

        auto_hide_desc = QLabel("Automatically closes the window after copying an item")
        auto_hide_desc.setObjectName("noteLabel")
        auto_hide_desc.setContentsMargins(20, 0, 0, 5)
        layout.addWidget(auto_hide_desc)

        # Auto-paste option
        self.auto_paste_checkbox = QCheckBox("Auto-paste after copying")
        auto_paste_enabled = self.settings_manager.get("auto_paste_on_copy", False)
        self.auto_paste_checkbox.setChecked(auto_paste_enabled)
        self.auto_paste_checkbox.stateChanged.connect(self._handle_auto_paste_change)
        layout.addWidget(self.auto_paste_checkbox)

        auto_paste_desc = QLabel(
            "Automatically pastes the copied item\n"
            "  • X11: requires xdotool (sudo apt install xdotool)\n"
            "  • Wayland: requires wtype or ydotool (sudo apt install wtype)"
        )
        auto_paste_desc.setObjectName("noteLabel")
        auto_paste_desc.setContentsMargins(20, 0, 0, 0)
        auto_paste_desc.setWordWrap(True)
        layout.addWidget(auto_paste_desc)

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

    def _handle_auto_hide_change(self, state: int) -> None:
        """Handle auto-hide checkbox change.

        Args:
            state: Checkbox state
        """
        enabled = state == Qt.CheckState.Checked.value
        self.settings_manager.set("auto_hide_on_copy", enabled)
        self.settings_manager.save()

    def _handle_auto_paste_change(self, state: int) -> None:
        """Handle auto-paste checkbox change.

        Args:
            state: Checkbox state
        """
        enabled = state == Qt.CheckState.Checked.value
        self.settings_manager.set("auto_paste_on_copy", enabled)
        self.settings_manager.save()

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
