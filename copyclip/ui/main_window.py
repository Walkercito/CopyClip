"""Main application window."""

import contextlib
import subprocess
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
from copyclip.utils.environment import detect_paste_tool, get_session_type
from copyclip.utils.single_instance import SingleInstance

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
        self.single_instance = SingleInstance()

        # Detect and cache environment settings
        self._cache_environment_settings()

        # Window state
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

        # Start clipboard monitoring
        self._start_clipboard_timer()
        self._start_signal_timer()

        self.hide()

    def set_hotkey_manager(self, hotkey_manager) -> None:
        """Set the hotkey manager instance.

        Args:
            hotkey_manager: HotkeyManager instance
        """
        self.hotkey_manager = hotkey_manager

    def _cache_environment_settings(self) -> None:
        """Detect and cache window manager and paste tool to avoid repeated detection."""
        # Cache window manager if not already cached
        if self.settings_manager.get("window_manager") is None:
            wm = get_session_type()
            self.settings_manager.set("window_manager", wm)
            self.settings_manager.save()
            print(f"Detected window manager: {wm}")

        # Cache paste tool if not already cached
        if self.settings_manager.get("paste_tool") is None:
            tool = detect_paste_tool()
            self.settings_manager.set("paste_tool", tool)
            self.settings_manager.save()
            if tool:
                print(f"Detected paste tool: {tool}")
            else:
                print("No paste tool found (xdotool/wtype/ydotool)")

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
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

        main_layout.addWidget(self._create_header_bar())

        content_widget = QWidget()
        content_layout = QVBoxLayout(content_widget)
        content_layout.setContentsMargins(16, 16, 16, 16)
        content_layout.setSpacing(12)

        content_layout.addWidget(self._create_search_section())
        content_layout.addWidget(self._create_clips_section(), 1)
        content_layout.addWidget(self._create_footer_section())

        main_layout.addWidget(content_widget)

        theme = self.settings_manager.get("theme", Theme.DARK)
        self.apply_theme(theme)

    def _create_header_bar(self) -> QFrame:
        """Create the header bar with title and actions.

        Returns:
            Header bar widget
        """
        header = QFrame()
        header.setObjectName("headerBar")
        header_layout = QHBoxLayout(header)
        header_layout.setContentsMargins(16, 12, 16, 12)
        header_layout.setSpacing(12)

        # Title
        title_label = QLabel(APP_NAME)
        title_label.setObjectName("headerTitle")
        header_layout.addWidget(title_label)

        header_layout.addStretch(1)

        # Actions
        settings_btn = QPushButton("Settings")
        settings_btn.setToolTip("Open application settings")
        settings_btn.setObjectName("headerButton")
        settings_btn.clicked.connect(self.show_settings)
        header_layout.addWidget(settings_btn)

        kill_btn = QPushButton("Quit")
        kill_btn.setToolTip("Close CopyClip completely")
        kill_btn.setObjectName("killButton")
        kill_btn.clicked.connect(self._kill_application)
        header_layout.addWidget(kill_btn)

        return header

    def _create_search_section(self) -> QFrame:
        """Create the search section with label.

        Returns:
            Search section widget
        """
        section = QFrame()
        section_layout = QVBoxLayout(section)
        section_layout.setContentsMargins(0, 0, 0, 0)
        section_layout.setSpacing(8)

        # Search input
        self.search_entry = QLineEdit()
        self.search_entry.setPlaceholderText("Search clipboard history...")
        self.search_entry.textChanged.connect(self._filter_clips)
        self.search_entry.setObjectName("searchEntry")
        section_layout.addWidget(self.search_entry)

        return section

    def _create_clips_section(self) -> QFrame:
        """Create the clips section with history items.

        Returns:
            Clips section widget
        """
        section = QFrame()
        section.setObjectName("clipsSection")
        section_layout = QVBoxLayout(section)
        section_layout.setContentsMargins(0, 0, 0, 0)
        section_layout.setSpacing(8)

        # Section header
        header_layout = QHBoxLayout()
        header_layout.setContentsMargins(0, 0, 0, 0)

        clips_title = QLabel("History")
        clips_title.setObjectName("sectionTitle")
        header_layout.addWidget(clips_title)

        header_layout.addStretch(1)

        clear_btn = QPushButton("Clear")
        clear_btn.setToolTip("Remove all non-pinned items")
        clear_btn.setObjectName("secondaryButton")
        clear_btn.clicked.connect(self._clear_history)
        header_layout.addWidget(clear_btn)

        section_layout.addLayout(header_layout)

        # Scroll area
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
        self.clips_layout.setContentsMargins(0, 0, 0, 0)

        # Add status labels
        self.empty_label = QLabel("No clipboard history yet\nCopy something to get started")
        self.empty_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.empty_label.setObjectName("emptyHistoryLabel")
        self.empty_label.setWordWrap(True)
        self.clips_layout.addWidget(self.empty_label)
        self.empty_label.hide()

        self.no_results_label = QLabel("No results found")
        self.no_results_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.no_results_label.setObjectName("noResultsLabel")
        self.no_results_label.setWordWrap(True)
        self.clips_layout.addWidget(self.no_results_label)
        self.no_results_label.hide()

        section_layout.addWidget(self.clips_scroll_area, 1)

        return section

    def _create_footer_section(self) -> QFrame:
        """Create the footer section with instructions.

        Returns:
            Footer section widget
        """
        footer = QFrame()
        footer.setObjectName("footer")
        footer_layout = QVBoxLayout(footer)
        footer_layout.setContentsMargins(0, 8, 0, 0)
        footer_layout.setSpacing(4)

        instructions = QLabel("Left Click to copy • Ctrl+Click to pin")
        instructions.setAlignment(Qt.AlignmentFlag.AlignCenter)
        instructions.setObjectName("instructionsLabel")
        footer_layout.addWidget(instructions)

        return footer

    def _start_clipboard_timer(self) -> None:
        """Start the clipboard monitoring timer."""
        self.clipboard_timer = QTimer(self)
        self.clipboard_timer.timeout.connect(self._check_clipboard_updates)
        interval = self.settings_manager.get("clipboard_check_interval", 1000)
        self.clipboard_timer.start(interval)

    def _start_signal_timer(self) -> None:
        """Start the signal monitoring timer for single instance."""
        self.signal_timer = QTimer(self)
        self.signal_timer.timeout.connect(self._check_show_signal)
        self.signal_timer.start(500)  # Check every 500ms

    @pyqtSlot()
    def _check_show_signal(self) -> None:
        """Check for signal to show window from another instance."""
        if self.single_instance.check_show_signal():
            self.show_clipboard()

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
        max_chars = self.settings_manager.get("max_chars_display", 100)
        clip_frame = ClipFrame(item, max_chars=max_chars)
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

                # Auto-paste if enabled
                auto_paste = self.settings_manager.get("auto_paste_on_copy", False)
                if auto_paste:
                    self._perform_auto_paste()

                # Auto-hide window based on user preference
                auto_hide = self.settings_manager.get("auto_hide_on_copy", True)
                if auto_hide:
                    delay = self.settings_manager.get("clipboard_auto_hide_delay", 800)
                    QTimer.singleShot(delay, self.hide_clipboard)
                return True
            self.show_feedback("Failed to copy content", FeedbackType.ERROR)
            return False
        except Exception as e:
            print(f"Error copying to clipboard: {e}")
            self.show_feedback(f"Error copying: {str(e)}", FeedbackType.ERROR)
            return False

    def _perform_auto_paste(self) -> None:
        """Perform auto-paste using xdotool (X11) or ydotool (Wayland)."""
        try:
            # Hide window first and wait for focus to return to previous app
            self.hide()
            # Delay before pasting for window to hide + focus to restore
            delay = self.settings_manager.get("auto_paste_delay", 200)
            QTimer.singleShot(delay, self._execute_paste_command)
        except Exception as e:
            print(f"Error scheduling auto-paste: {e}")

    def _execute_paste_command(self) -> None:
        """Execute the paste command using cached paste tool."""
        paste_tool = self.settings_manager.get("paste_tool")

        if not paste_tool:
            wm = self.settings_manager.get("window_manager", "unknown")
            print(f"No paste tool available for {wm}")
            self.show_feedback(
                f"Auto-paste not available. Install: "
                f"{'wtype or ydotool' if wm == 'wayland' else 'xdotool'}",
                FeedbackType.WARNING,
                4000,
            )
            return

        try:
            if paste_tool == "xdotool":
                subprocess.run(
                    ["xdotool", "key", "ctrl+v"],
                    check=True,
                    capture_output=True,
                    timeout=1,
                )
            elif paste_tool == "wtype":
                subprocess.run(
                    ["wtype", "-M", "ctrl", "-P", "v", "-m", "ctrl", "-p", "v"],
                    check=True,
                    capture_output=True,
                    timeout=1,
                )
            elif paste_tool == "ydotool":
                subprocess.run(
                    ["ydotool", "key", "ctrl+v"],
                    check=True,
                    capture_output=True,
                    timeout=1,
                )
        except FileNotFoundError:
            print(f"Paste tool {paste_tool} not found (was it uninstalled?)")
            # Re-detect paste tool
            self.settings_manager.set("paste_tool", None)
            self.settings_manager.save()
            self.show_feedback(
                f"{paste_tool} not found. Re-detecting on next start.", FeedbackType.WARNING, 4000
            )
        except subprocess.CalledProcessError as e:
            print(f"Paste command failed: {e}")
        except subprocess.TimeoutExpired:
            print("Auto-paste command timed out")
        except Exception as e:
            print(f"Error executing auto-paste: {e}")

    def show_feedback(
        self,
        message: str,
        type_: str = FeedbackType.INFO,
        duration: int = FEEDBACK_DURATION,
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

    @pyqtSlot()
    def _kill_application(self) -> None:
        """Terminate the application completely."""
        self._force_quit = True
        QApplication.instance().quit()

    def show_clipboard(self) -> None:
        """Show the clipboard window."""
        self.update_clips_display()
        self.show()
        self.raise_()
        self.activateWindow()
        self.search_entry.clear()
        self.search_entry.setFocus()

    def hide_clipboard(self) -> None:
        """Hide the clipboard window."""
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
            self.single_instance.release_lock()
            event.accept()
            return

        event.ignore()
        self.hide_clipboard()
