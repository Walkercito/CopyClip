"""First run dialog for hotkey selection."""

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
    QButtonGroup,
    QDialog,
    QFrame,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QRadioButton,
    QVBoxLayout,
)

from copyclip.hotkeys.config import HotkeyConfig


class FirstRunDialog(QDialog):
    """Dialog shown on first run to select hotkey preference."""

    # Descriptions for each hotkey preset
    PRESET_DESCRIPTIONS = {
        "super_v": "  (Recommended - Similar to Windows clipboard)",
        "ctrl_alt_v": "  (Alternative if Super key is in use)",
        "super_c": "  (Alternative using C key)",
        "ctrl_shift_v": "  (Alternative using Shift modifier)",
    }

    def __init__(self, parent=None):
        """Initialize the first run dialog.

        Args:
            parent: Parent widget
        """
        super().__init__(parent)
        self.selected_preset = "super_v"  # Default
        self._setup_ui()

    def _setup_ui(self) -> None:  # noqa PLR0915
        """Setup the dialog UI."""
        self.setWindowTitle("Welcome to CopyClip")
        self.setModal(True)
        self.setMinimumWidth(500)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(20)

        # Welcome message (title)
        welcome_label = QLabel("Welcome to CopyClip!")
        welcome_font = QFont()
        welcome_font.setPointSize(16)
        welcome_font.setBold(True)
        welcome_label.setFont(welcome_font)
        welcome_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(welcome_label)

        # Description (subtitle)
        desc_label = QLabel("A clipboard manager that helps you manage your clipboard history")
        desc_label.setWordWrap(True)
        desc_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        desc_label.setObjectName("subtitleLabel")
        layout.addWidget(desc_label)

        selection_container = QFrame()
        selection_container.setObjectName("selectionContainer")
        selection_container.setFrameShape(QFrame.Shape.Box)
        selection_container.setLineWidth(1)

        container_layout = QVBoxLayout(selection_container)
        container_layout.setContentsMargins(20, 16, 20, 16)
        container_layout.setSpacing(12)

        instruction_label = QLabel("Please select a keyboard shortcut to open the clipboard:")
        instruction_label.setWordWrap(True)
        instruction_label.setObjectName("instructionLabel")
        container_layout.addWidget(instruction_label)

        hotkey_group = QButtonGroup(self)
        self.radio_buttons = {}

        presets = HotkeyConfig.get_all_presets()
        for preset_name, binding in presets.items():
            radio = QRadioButton()
            radio.setObjectName(f"hotkey_{preset_name}")

            description = self.PRESET_DESCRIPTIONS.get(preset_name, "")
            label_text = f"{binding.display_name}{description}"
            radio.setText(label_text)

            self.radio_buttons[preset_name] = radio
            hotkey_group.addButton(radio)
            container_layout.addWidget(radio)

        # Set default selection
        self.radio_buttons["super_v"].setChecked(True)

        layout.addWidget(selection_container)

        note_label = QLabel("Note: You can change this hotkey later in Settings.")
        note_label.setWordWrap(True)
        note_font = QFont()
        note_font.setPointSize(9)
        note_font.setItalic(True)
        note_label.setFont(note_font)
        note_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        note_label.setObjectName("noteLabel")
        layout.addWidget(note_label)

        button_layout = QHBoxLayout()
        button_layout.addStretch(1)

        confirm_button = QPushButton("Continue")
        confirm_button.setDefault(True)
        confirm_button.clicked.connect(self._on_confirm)
        button_layout.addWidget(confirm_button)

        layout.addLayout(button_layout)

    def _on_confirm(self) -> None:
        """Handle confirm button click."""
        for preset_name, radio in self.radio_buttons.items():
            if radio.isChecked():
                self.selected_preset = preset_name
                break

        self.accept()

    def get_selected_preset(self) -> str:
        """Get the selected hotkey preset name.

        Returns:
            Preset name (e.g., 'super_v')
        """
        return self.selected_preset
