"""Clipboard item frame widget."""

from datetime import datetime

from PyQt6.QtCore import QPointF, Qt, pyqtSignal
from PyQt6.QtGui import QCursor, QFont, QMouseEvent
from PyQt6.QtWidgets import (
    QApplication,
    QFrame,
    QHBoxLayout,
    QLabel,
    QSizePolicy,
    QVBoxLayout,
)

from copyclip.core.history import HistoryItem
from copyclip.ui.widgets.clickable_label import ClickableLabel
from copyclip.utils.constants import MAX_CHARS_DISPLAY


class ClipFrame(QFrame):
    """Represents a single clipboard item with truncation and expansion support."""

    copy_clicked = pyqtSignal(str)
    pin_clicked = pyqtSignal(str)

    def __init__(self, item: HistoryItem, parent=None, max_chars: int = MAX_CHARS_DISPLAY):
        """Initialize the clip frame.

        Args:
            item: History item dictionary
            parent: Parent widget
            max_chars: Maximum characters to display before truncation
        """
        super().__init__(parent)

        self._validate_and_extract_item(item)
        self.max_chars = max_chars
        self.is_content_expanded = False

        self._setup_frame()
        self._create_layout()
        self.update_content_display()

    def _validate_and_extract_item(self, item: HistoryItem) -> None:
        """Validate and extract data from history item.

        Args:
            item: History item to validate
        """
        required_fields = ["content", "pinned", "timestamp"]
        if not all(field in item for field in required_fields):
            print(f"Warning: Invalid item format: {item}. Using defaults.")
            self.original_content = item.get("content", "Invalid Content")
            self.pinned = item.get("pinned", False)
            self.timestamp = item.get("timestamp", datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        else:
            self.original_content = item["content"]
            self.pinned = item["pinned"]
            self.timestamp = item["timestamp"]

    def _setup_frame(self) -> None:
        """Setup frame appearance."""
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setFrameShadow(QFrame.Shadow.Raised)
        self.setObjectName("clipFrame")
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

    def _create_layout(self) -> None:
        """Create the widget layout."""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 6, 8, 6)
        layout.setSpacing(4)

        top_row_layout = self._create_top_row()
        layout.addLayout(top_row_layout)

        self.content_label = ClickableLabel("", self)
        self.content_label.setWordWrap(True)
        self.content_label.setTextInteractionFlags(
            Qt.TextInteractionFlag.TextSelectableByMouse
            | Qt.TextInteractionFlag.TextSelectableByKeyboard
        )
        self.content_label.setObjectName("contentLabel")
        self.content_label.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)
        layout.addWidget(self.content_label)

        # Toggle expand/collapse label
        self.toggle_content_label = QLabel(self)
        self.toggle_content_label.setObjectName("toggleContentLabel")
        self.toggle_content_label.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.toggle_content_label.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.toggle_content_label.mousePressEvent = self._handle_toggle_click
        layout.addWidget(self.toggle_content_label)

    def _create_top_row(self) -> QHBoxLayout:
        """Create the top row with pin indicator and timestamp.

        Returns:
            QHBoxLayout containing the top row widgets
        """
        top_row_layout = QHBoxLayout()
        top_row_layout.setContentsMargins(0, 0, 0, 0)
        top_row_layout.setSpacing(5)

        if self.pinned:
            pin_indicator = QLabel("📌", self)
            pin_indicator.setObjectName("pinIndicator")
            font_pin = QFont()
            font_pin.setPointSize(10)
            pin_indicator.setFont(font_pin)
            top_row_layout.addWidget(pin_indicator, alignment=Qt.AlignmentFlag.AlignLeft)
        else:
            top_row_layout.addStretch(0)

        time_label = ClickableLabel(self.timestamp, self)
        time_label.setObjectName("timeLabel")
        font_time = QFont()
        font_time.setPointSize(8)
        time_label.setFont(font_time)
        time_label.setAlignment(Qt.AlignmentFlag.AlignRight)
        top_row_layout.addWidget(time_label, 1, alignment=Qt.AlignmentFlag.AlignRight)

        return top_row_layout

    def update_content_display(self) -> None:
        """Update the content label text based on expanded state."""
        is_truncated = len(self.original_content) > self.max_chars

        if self.is_content_expanded:
            self.content_label.setText(self.original_content)
            if is_truncated:
                self.toggle_content_label.setText("Show less")
                self.toggle_content_label.show()
            else:
                self.toggle_content_label.hide()
        elif is_truncated:
            display_text = self.original_content[: self.max_chars] + "..."
            self.content_label.setText(display_text)
            self.toggle_content_label.setText("Show all")
            self.toggle_content_label.show()
        else:
            self.content_label.setText(self.original_content)
            self.toggle_content_label.hide()

        self.layout().activate()
        self.adjustSize()
        self.updateGeometry()

    def _handle_toggle_click(self, event: QMouseEvent) -> None:
        """Handle clicks on the 'Show all'/'Show less' label.

        Args:
            event: Mouse event
        """
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_content_expanded = not self.is_content_expanded
            self.update_content_display()
            event.accept()
        else:
            event.ignore()

    def mousePressEvent(self, event: QMouseEvent) -> None:
        """Handle clicks on the frame to copy or pin.

        Args:
            event: Mouse event
        """
        # Check if click is on toggle label
        toggle_rect = self.toggle_content_label.geometry()
        if self.toggle_content_label.isVisible() and toggle_rect.contains(event.pos()):
            mapped_pos = self.toggle_content_label.mapFromParent(event.pos())
            mapped_pos_f = QPointF(mapped_pos)
            label_event = QMouseEvent(
                event.type(),
                mapped_pos_f,
                event.globalPosition(),
                event.button(),
                event.buttons(),
                event.modifiers(),
            )
            self._handle_toggle_click(label_event)
            return

        content_rect = self.content_label.geometry()
        is_click_on_selectable_content = (
            content_rect.contains(event.pos()) and self.content_label.hasSelectedText()
        )

        if not is_click_on_selectable_content:
            modifiers = QApplication.keyboardModifiers()
            if event.button() == Qt.MouseButton.LeftButton:
                if modifiers == Qt.KeyboardModifier.ControlModifier:
                    self.pin_clicked.emit(self.original_content)
                elif modifiers == Qt.KeyboardModifier.NoModifier:
                    self.copy_clicked.emit(self.original_content)
                return

        super().mousePressEvent(event)
