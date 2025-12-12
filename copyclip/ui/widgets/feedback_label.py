"""Feedback label widget for displaying temporary messages."""

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import QLabel

from copyclip.utils.constants import FeedbackType


class FeedbackLabel(QLabel):
    """A temporary label for showing user feedback messages."""

    # Color schemes for different feedback types
    COLORS = {
        FeedbackType.SUCCESS: "background-color: #28a745; color: white;",
        FeedbackType.ERROR: "background-color: #dc3545; color: white;",
        FeedbackType.WARNING: "background-color: #ffc107; color: black;",
        FeedbackType.INFO: "background-color: #17a2b8; color: white;",
    }

    def __init__(self, message: str, type_: str = FeedbackType.INFO, parent=None):
        """Initialize the feedback label.

        Args:
            message: Message to display
            type_: Type of feedback (success, error, warning, info)
            parent: Parent widget
        """
        super().__init__(message, parent)

        style = self.COLORS.get(type_, self.COLORS[FeedbackType.INFO])
        self.setStyleSheet(
            f"""
            QLabel {{
                padding: 8px 20px;
                border-radius: 8px;
                {style}
                border: 1px solid rgba(0, 0, 0, 0.1);
            }}
        """
        )

        self.setAlignment(Qt.AlignmentFlag.AlignCenter)

        font = QFont()
        font.setBold(True)
        self.setFont(font)

        self.raise_()
        self.adjustSize()
