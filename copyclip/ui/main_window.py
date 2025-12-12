"""Main application window."""

import contextlib
from typing import TYPE_CHECKING

from PyQt6.QtCore import Qt, QTimer, pyqtSlot
from PyQt6.QtGui import QKeySequence, QShortcut
from PyQt6.QtWidgets import (
    QApplication,
    QFrame,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from copyclip.core.history import HistoryItem
from copyclip.ui.dialogs.settings import SettingsDialog
from copyclip.ui.styles.themes import ThemeManager
from copyclip.ui.widgets import ClipFrame, FeedbackLabel
from copyclip.utils.constants import (
    APP_NAME,
    CLIPBOARD_AUTO_HIDE_DELAY,
    CLIPBOARD_CHECK_INTERVAL,
    FEEDBACK_DURATION,
    FEEDBACK_DURATION_SHORT,
    SHORTCUT_HIDE,
    SHORTCUT_QUIT,
    WINDOW_DEFAULT_HEIGHT,
    WINDOW_DEFAULT_WIDTH,
    WINDOW_MIN_HEIGHT,
    WINDOW_MIN_WIDTH,
    FeedbackType,
    Theme,
)

if TYPE_CHECKING:
    from copyclip.core.history import HistoryManager
    from copyclip.core.settings import SettingsManager


class MainWindow(QMainWindow):
    """Main application window and UI manager."""

    def __init__(
        self,
        history_manager: "HistoryManager",
        settings_manager: "SettingsManager",
    ):
        """Initialize the main window.

        Args:
            history_manager: History manager instance
            settings_manager: Settings manager instance
        """
        super().__init__()

        if history_manager is None or settings_manager is None:
            raise ValueError("MainWindow requires valid manager instances")

        self.history_manager = history_manager
        self.settings_manager = settings_manager
        self.hotkey_manager = None  # Will be set by main.py

        # Window state
        self.window_pinned = False
        self._force_quit = False

        # UI components
        self.clip_frames: list[ClipFrame] = []
        self.empty_label: QLabel | None = None
        self.no_results_label: QLabel | None = None
        self._feedback_timer: QTimer | None = None
        self._feedback_label: FeedbackLabel | None = None

        # Initialize UI
        self._setup_window()
        self._create_gui()
        self._setup_shortcuts()
        self._apply_saved_state()

        # Start clipboard monitoring
        self._start_clipboard_timer()

        self.hide()

    def set_hotkey_manager(self, hotkey_manager) -> None:
        """Set the hotkey manager instance.

        Args:
            hotkey_manager: HotkeyManager instance
        """
        self.hotkey_manager = hotkey_manager

    def _setup_window(self) -> None:
        """Setup window properties."""
        self.setWindowTitle(APP_NAME)
        self.setMinimumSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT)
        self.resize(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT)

    def _setup_shortcuts(self) -> None:
        """Setup keyboard shortcuts."""
        QShortcut(QKeySequence(SHORTCUT_HIDE), self).activated.connect(self.hide_clipboard)
        QShortcut(QKeySequence(SHORTCUT_QUIT), self).activated.connect(self._terminate_application)

    @pyqtSlot()
    def _terminate_application(self) -> None:
        """Terminate the application (triggered by Ctrl+Esc)."""
        if self.isVisible():
            self._force_quit = True
            QApplication.instance().quit()

    def _create_gui(self) -> None:
        """Create the main GUI elements."""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)

        main_layout.addWidget(self._create_search_frame())
        main_layout.addWidget(self._create_clips_scroll_area(), 1)
        main_layout.addWidget(self._create_instructions_label())
        main_layout.addWidget(self._create_button_frame())

        theme = self.settings_manager.get("theme", Theme.DARK)
        self.apply_theme(theme)

    def _create_search_frame(self) -> QFrame:
        """Create the search input frame.

        Returns:
            Search frame widget
        """
        search_frame = QFrame()
        search_layout = QHBoxLayout(search_frame)
        search_layout.setContentsMargins(0, 0, 0, 0)
        search_layout.setSpacing(5)

        self.search_entry = QLineEdit()
        self.search_entry.setPlaceholderText("Search clips...")
        self.search_entry.textChanged.connect(self._filter_clips)
        self.search_entry.setObjectName("searchEntry")

        clear_button = QPushButton("×")
        clear_button.setFixedSize(28, 28)
        clear_button.setToolTip("Clear search")
        clear_button.setObjectName("clearSearchButton")
        clear_button.clicked.connect(self.search_entry.clear)

        search_layout.addWidget(self.search_entry)
        search_layout.addWidget(clear_button)

        return search_frame

    def _create_clips_scroll_area(self) -> QScrollArea:
        """Create the clips scroll area.

        Returns:
            Scroll area widget
        """
        self.clips_scroll_area = QScrollArea()
        self.clips_scroll_area.setWidgetResizable(True)
        self.clips_scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        self.clips_scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.clips_scroll_area.setFrameShape(QFrame.Shape.NoFrame)
        self.clips_scroll_area.setObjectName("clipsScrollArea")

        self.clips_content_widget = QWidget()
        self.clips_content_widget.setObjectName("clips_content_widget")
        self.clips_scroll_area.setWidget(self.clips_content_widget)

        self.clips_layout = QVBoxLayout(self.clips_content_widget)
        self.clips_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        self.clips_layout.setSpacing(6)
        self.clips_layout.setContentsMargins(5, 5, 5, 5)

        # Add status labels
        self.empty_label = QLabel("Clipboard history is empty.")
        self.empty_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.empty_label.setObjectName("emptyHistoryLabel")
        self.empty_label.setWordWrap(True)
        self.clips_layout.addWidget(self.empty_label)
        self.empty_label.hide()

        self.no_results_label = QLabel("No results found matching your search.")
        self.no_results_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.no_results_label.setObjectName("noResultsLabel")
        self.no_results_label.setWordWrap(True)
        self.clips_layout.addWidget(self.no_results_label)
        self.no_results_label.hide()

        return self.clips_scroll_area

    def _create_instructions_label(self) -> QLabel:
        """Create the instructions label.

        Returns:
            Instructions label widget
        """
        instructions = QLabel("Left Click: Copy | Ctrl+Click: Toggle Pin")
        instructions.setAlignment(Qt.AlignmentFlag.AlignCenter)
        instructions.setObjectName("instructionsLabel")
        return instructions

    def _create_button_frame(self) -> QFrame:
        """Create the button frame.

        Returns:
            Button frame widget
        """
        button_frame = QFrame()
        button_layout = QHBoxLayout(button_frame)
        button_layout.setContentsMargins(0, 0, 0, 0)
        button_layout.setSpacing(10)

        clear_btn = QPushButton("Clear History")
        clear_btn.setToolTip("Remove all non-pinned items")
        clear_btn.clicked.connect(self._clear_history)

        settings_btn = QPushButton("Settings")
        settings_btn.setToolTip("Open application settings")
        settings_btn.clicked.connect(self.show_settings)

        self.pin_button = QPushButton("Pin Window")
        self.pin_button.setToolTip("Keep window on top and open")
        self.pin_button.setCheckable(True)
        self.pin_button.toggled.connect(self._toggle_pin)

        button_layout.addStretch(1)
        button_layout.addWidget(clear_btn)
        button_layout.addWidget(settings_btn)
        button_layout.addWidget(self.pin_button)
        button_layout.addStretch(1)

        return button_frame

    def _apply_saved_state(self) -> None:
        """Apply saved window state from settings."""
        is_pinned = self.settings_manager.get("window_pinned", False)
        self.window_pinned = is_pinned
        self.pin_button.setChecked(is_pinned)
        self.pin_button.setText("Unpin Window" if is_pinned else "Pin Window")
        self.setWindowFlag(Qt.WindowType.WindowStaysOnTopHint, is_pinned)

    def _start_clipboard_timer(self) -> None:
        """Start the clipboard monitoring timer."""
        self.clipboard_timer = QTimer(self)
        self.clipboard_timer.timeout.connect(self._check_clipboard_updates)
        self.clipboard_timer.start(CLIPBOARD_CHECK_INTERVAL)

    @pyqtSlot()
    def _check_clipboard_updates(self) -> None:
        """Check for clipboard updates."""
        try:
            content = self.history_manager.clipboard_manager.check_for_changes()
            if content and self.history_manager.add_item(content):
                self.update_clips_display()
        except AttributeError:
            print("Error: Managers not properly configured")
            self.clipboard_timer.stop()
        except Exception as e:
            print(f"Error checking clipboard: {e}")

    def update_clips_display(self) -> None:
        """Clear and redraw the list of clips."""
        self._clear_clip_frames()

        # Load history
        try:
            history_items = self.history_manager.get_history()
        except Exception as e:
            print(f"Error getting history: {e}")
            self.show_feedback("Error loading history", FeedbackType.ERROR)
            history_items = []

        self.empty_label.hide()
        self.no_results_label.hide()

        if history_items:
            for item in history_items:
                try:
                    self._add_clip_frame(item)
                except Exception as e:
                    print(f"Error creating ClipFrame: {e}")
        else:
            self.empty_label.show()

        self._filter_clips()

    def _clear_clip_frames(self) -> None:
        """Clear all clip frames from the layout."""
        items_to_remove = []
        widgets_to_delete = []

        for i in range(self.clips_layout.count()):
            item = self.clips_layout.itemAt(i)
            widget = item.widget()
            if isinstance(widget, ClipFrame):
                items_to_remove.append(item)
                widgets_to_delete.append(widget)

                with contextlib.suppress(TypeError, AttributeError):
                    widget.copy_clicked.disconnect(self._handle_copy_clicked)
                with contextlib.suppress(TypeError, AttributeError):
                    widget.pin_clicked.disconnect(self._handle_pin_clicked)

        for item in items_to_remove:
            self.clips_layout.removeItem(item)

        for widget in widgets_to_delete:
            widget.setParent(None)
            widget.deleteLater()

        self.clip_frames.clear()

    def _add_clip_frame(self, item: HistoryItem) -> None:
        """Add a clip frame to the layout.

        Args:
            item: History item to display
        """
        clip_frame = ClipFrame(item)
        clip_frame.copy_clicked.connect(self._handle_copy_clicked)
        clip_frame.pin_clicked.connect(self._handle_pin_clicked)

        insertion_index = len(self.clip_frames)
        self.clips_layout.insertWidget(insertion_index, clip_frame)
        self.clip_frames.append(clip_frame)

    @pyqtSlot(str)
    def _handle_copy_clicked(self, content: str) -> None:
        """Handle copy signal from ClipFrame.

        Args:
            content: Content to copy
        """
        self._copy_to_clipboard(content)

    @pyqtSlot(str)
    def _handle_pin_clicked(self, content: str) -> None:
        """Handle pin toggle signal from ClipFrame.

        Args:
            content: Content to toggle pin for
        """
        try:
            is_pinned = self.history_manager.toggle_pin(content)
            self.update_clips_display()
            status = "pinned" if is_pinned else "unpinned"
            self.show_feedback(f"Item {status}", FeedbackType.INFO, FEEDBACK_DURATION_SHORT)
        except Exception as e:
            print(f"Error toggling pin: {e}")
            self.show_feedback("Error updating pin status", FeedbackType.ERROR)

    def _copy_to_clipboard(self, content: str) -> bool:
        """Copy content to clipboard.

        Args:
            content: Content to copy

        Returns:
            True if successful, False otherwise
        """
        if not content:
            self.show_feedback("Nothing to copy", FeedbackType.WARNING)
            return False

        try:
            success = self.history_manager.clipboard_manager.set_content(content)
            if success:
                self.show_feedback("Copied successfully!", FeedbackType.SUCCESS)
                if not self.window_pinned:
                    QTimer.singleShot(CLIPBOARD_AUTO_HIDE_DELAY, self.hide_clipboard)
                return True
            self.show_feedback("Failed to copy content", FeedbackType.ERROR)
            return False
        except Exception as e:
            print(f"Error copying to clipboard: {e}")
            self.show_feedback(f"Error copying: {str(e)}", FeedbackType.ERROR)
            return False

    def show_feedback(
        self, message: str, type_: str = FeedbackType.INFO, duration: int = FEEDBACK_DURATION
    ) -> None:
        """Display a temporary feedback message.

        Args:
            message: Message to display
            type_: Feedback type (success, error, warning, info)
            duration: Duration in milliseconds
        """
        if self._feedback_label:
            self._feedback_label.deleteLater()
        if self._feedback_timer:
            self._feedback_timer.stop()

        self._feedback_label = FeedbackLabel(message, type_, self.centralWidget())
        label_size = self._feedback_label.sizeHint()
        x = (self.width() - label_size.width()) // 2
        y = self.height() - label_size.height() - 15
        self._feedback_label.move(max(0, x), max(0, y))
        self._feedback_label.show()
        self._feedback_label.raise_()

        if not self._feedback_timer:
            self._feedback_timer = QTimer(self)
            self._feedback_timer.setSingleShot(True)
            self._feedback_timer.timeout.connect(self._hide_feedback)

        self._feedback_timer.start(duration)

    def _hide_feedback(self) -> None:
        """Hide the feedback label."""
        if self._feedback_label:
            self._feedback_label.deleteLater()
            self._feedback_label = None
        if self._feedback_timer:
            self._feedback_timer.stop()

    def _filter_clips(self) -> None:
        """Show or hide clips based on search query."""
        search_text = self.search_entry.text().lower().strip()
        has_visible_items = False
        history_has_items = False

        self.empty_label.hide()
        self.no_results_label.hide()

        for frame in self.clip_frames:
            history_has_items = True
            matches = search_text in frame.original_content.lower()
            frame.setVisible(matches)
            if matches:
                has_visible_items = True

        if not history_has_items:
            self.empty_label.show()
        elif search_text and not has_visible_items:
            self.no_results_label.show()

    def _clear_history(self) -> None:
        """Clear non-pinned items from history."""
        reply = QMessageBox.question(
            self,
            "Confirm Clear",
            "Are you sure you want to clear all non-pinned items?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if reply == QMessageBox.StandardButton.Yes:
            try:
                self.history_manager.clear_unpinned()
                self.update_clips_display()
                self.show_feedback("History cleared (pinned items kept)", FeedbackType.INFO)
            except Exception as e:
                print(f"Error clearing history: {e}")
                self.show_feedback("Error clearing history", FeedbackType.ERROR)

    @pyqtSlot(bool)
    def _toggle_pin(self, checked: bool) -> None:
        """Toggle window pin state.

        Args:
            checked: New pin state
        """
        try:
            self.window_pinned = checked
            flags = self.windowFlags()

            if self.window_pinned:
                self.setWindowFlags(flags | Qt.WindowType.WindowStaysOnTopHint)
                self.pin_button.setText("Unpin Window")
            else:
                self.setWindowFlags(flags & ~Qt.WindowType.WindowStaysOnTopHint)
                self.pin_button.setText("Pin Window")

            self.show()
            self.settings_manager.set("window_pinned", self.window_pinned)
            self.settings_manager.save()
        except Exception as e:
            print(f"Error in toggle_pin: {e}")
            # Revert state
            self.pin_button.setChecked(not checked)

    def show_clipboard(self) -> None:
        """Show the clipboard window."""
        self.update_clips_display()
        self.show()
        self.raise_()
        self.activateWindow()
        self.search_entry.clear()
        self.search_entry.setFocus()

    def hide_clipboard(self) -> None:
        """Hide the clipboard window if not pinned."""
        if not self.window_pinned:
            self.hide()
            self.search_entry.clear()

    def apply_theme(self, theme: str) -> None:
        """Apply a visual theme.

        Args:
            theme: Theme name (dark, light, or system)
        """
        stylesheet = ThemeManager.get_theme_stylesheet(theme)
        self.setStyleSheet(stylesheet)
        self.settings_manager.set("theme", theme)
        self.settings_manager.save()

    def show_settings(self) -> None:
        """Display the settings dialog."""

        def on_theme_changed(theme: str) -> None:
            self.apply_theme(theme)

        def on_hotkey_changed(preset: str) -> None:
            if self.hotkey_manager:
                success = self.hotkey_manager.update_hotkey(preset)
                if success:
                    self.show_feedback("Hotkey updated", FeedbackType.SUCCESS)
                else:
                    self.show_feedback("Failed to update hotkey", FeedbackType.ERROR)

        dialog = SettingsDialog(self.settings_manager, on_theme_changed, on_hotkey_changed, self)
        dialog.exec()

    def closeEvent(self, event) -> None:
        """Handle window close event.

        Args:
            event: Close event
        """
        if self._force_quit:
            event.accept()
            return

        if self.window_pinned:
            event.ignore()
            self.show_feedback("Window is pinned. Unpin to close.", FeedbackType.WARNING, 2500)
        else:
            event.ignore()
            self.hide_clipboard()
