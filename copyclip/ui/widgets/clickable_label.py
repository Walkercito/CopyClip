"""Clickable label widget."""

from PyQt6.QtCore import QPointF, Qt
from PyQt6.QtGui import QCursor, QMouseEvent
from PyQt6.QtWidgets import QApplication, QLabel


class ClickableLabel(QLabel):
    """A QLabel that propagates mouse clicks to its parent for non-selection clicks."""

    def __init__(self, text: str = "", parent=None):
        """Initialize the clickable label.

        Args:
            text: Label text
            parent: Parent widget
        """
        super().__init__(text, parent)
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

    def mousePressEvent(self, event: QMouseEvent) -> None:
        """Handle mouse press events.

        Args:
            event: Mouse event
        """
        super().mousePressEvent(event)

        # Only propagate if no text is selected
        if not self.hasSelectedText():
            parent_widget = self.parent()
            while parent_widget and parent_widget.__class__.__name__ != "ClipFrame":
                parent_widget = parent_widget.parent()

            if parent_widget and parent_widget.__class__.__name__ == "ClipFrame":
                parent_pos = self.mapToParent(event.pos())
                parent_pos_f = QPointF(parent_pos)
                parent_event = QMouseEvent(
                    event.type(),
                    parent_pos_f,
                    event.globalPosition(),
                    event.button(),
                    event.buttons(),
                    event.modifiers(),
                )
                QApplication.sendEvent(parent_widget, parent_event)
