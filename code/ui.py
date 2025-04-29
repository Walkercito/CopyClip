import sys
import os
import json
from datetime import datetime
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
QHBoxLayout, QFrame, QLabel, QPushButton, QSizePolicy,
QLineEdit, QScrollArea, QMessageBox, QComboBox, QDialog)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QSize, pyqtSlot, QRect, QPointF, QEvent
from PyQt6.QtGui import QFont, QIcon, QKeySequence, QShortcut, QCursor, QMouseEvent


class ClipFrame(QFrame):
    """ Represents a single clipboard item, limited vertically by char count, horizontally wrapped. """
    copy_clicked = pyqtSignal(str)
    pin_clicked = pyqtSignal(str)
    MAX_CHARS_DISPLAY = 100 

    def __init__(self, item, parent=None):
        super().__init__(parent)
        if not isinstance(item, dict) or 'content' not in item or 'pinned' not in item or 'timestamp' not in item:
            print(f"Warning: Invalid item format received: {item}. Using defaults.")
            self.original_content = item.get('content', 'Invalid Content') 
            self.pinned = item.get('pinned', False)
            self.timestamp = item.get('timestamp', datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
        else:
            self.original_content = item['content']
            self.pinned = item['pinned']
            self.timestamp = item['timestamp']

        self.is_content_expanded = False 

        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setFrameShadow(QFrame.Shadow.Raised)
        self.setObjectName("clipFrame")
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))


        layout = QVBoxLayout(self)
        layout.setContentsMargins(8, 6, 8, 6)
        layout.setSpacing(4)

        top_row_layout = QHBoxLayout()
        top_row_layout.setContentsMargins(0,0,0,0)
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

        layout.addLayout(top_row_layout)


        self.content_label = ClickableLabel("", self) 
        self.content_label.setWordWrap(True)
        self.content_label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse | Qt.TextInteractionFlag.TextSelectableByKeyboard)
        self.content_label.setObjectName("contentLabel")
        self.content_label.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred) # Important for wrapping
        layout.addWidget(self.content_label)

        self.toggle_content_label = QLabel(self)
        self.toggle_content_label.setObjectName("toggleContentLabel")
        self.toggle_content_label.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.toggle_content_label.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.toggle_content_label.mousePressEvent = self.handle_toggle_click 
        layout.addWidget(self.toggle_content_label)

        self.update_content_display() 


    def update_content_display(self):
        """ Sets the content label text based on expanded state and original content length. """
        is_truncated = len(self.original_content) > self.MAX_CHARS_DISPLAY
        if self.is_content_expanded:
            display_text = self.original_content
            self.content_label.setText(display_text)
            if is_truncated: 
                 self.toggle_content_label.setText("Show less")
                 self.toggle_content_label.show()
            else:
                 self.toggle_content_label.hide() 
        else:
            if is_truncated:
                display_text = self.original_content[:self.MAX_CHARS_DISPLAY] + "..."
                self.content_label.setText(display_text)
                self.toggle_content_label.setText("Show all")
                self.toggle_content_label.show()
            else:
                display_text = self.original_content
                self.content_label.setText(display_text)
                self.toggle_content_label.hide()

        self.layout().activate()
        self.adjustSize() 
        self.updateGeometry() 


    def handle_toggle_click(self, event: QMouseEvent):
        """ Handles clicks on the 'Show all'/'Show less' label. """
        if event.button() == Qt.MouseButton.LeftButton:
            self.is_content_expanded = not self.is_content_expanded
            self.update_content_display()
            event.accept() # Consume the event so it doesn't propagate
        else:
            event.ignore()


    def mousePressEvent(self, event: QMouseEvent):
        """ Handles clicks on the frame to copy or pin, using original_content. """
        toggle_rect = self.toggle_content_label.geometry()
        if self.toggle_content_label.isVisible() and toggle_rect.contains(event.pos()):
            mapped_pos = self.toggle_content_label.mapFromParent(event.pos())
            mapped_pos_f = QPointF(mapped_pos)
            label_event = QMouseEvent(event.type(),
                                      mapped_pos_f,
                                      event.globalPosition(),
                                      event.button(),
                                      event.buttons(),
                                      event.modifiers())
            self.handle_toggle_click(label_event) 
            return 

        content_rect = self.content_label.geometry()
        is_click_on_selectable_content = content_rect.contains(event.pos()) and self.content_label.hasSelectedText()

        if not is_click_on_selectable_content:
            modifiers = QApplication.keyboardModifiers()
            if event.button() == Qt.MouseButton.LeftButton:
                if modifiers == Qt.KeyboardModifier.ControlModifier:
                    self.pin_clicked.emit(self.original_content)
                elif modifiers == Qt.KeyboardModifier.NoModifier:
                    self.copy_clicked.emit(self.original_content)
                return
        super().mousePressEvent(event)


class ClickableLabel(QLabel):
    """ A QLabel that propagates mouse clicks to its parent for non-selection clicks. """
    def __init__(self, text, parent=None):
        super().__init__(text, parent)
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

    def mousePressEvent(self, event: QMouseEvent):
        """ Handle text selection locally, pass other clicks to parent Frame. """
        super().mousePressEvent(event)

        if not self.hasSelectedText():
            parent_widget = self.parent()
            while parent_widget and not isinstance(parent_widget, ClipFrame):
                parent_widget = parent_widget.parent()

            if isinstance(parent_widget, ClipFrame):
                parent_pos = self.mapToParent(event.pos())
                parent_pos_f = QPointF(parent_pos)
                parent_event = QMouseEvent(event.type(),
                                           parent_pos_f, # Use QPointF
                                           event.globalPosition(), # Use QPointF
                                           event.button(),
                                           event.buttons(),
                                           event.modifiers())
                QApplication.sendEvent(parent_widget, parent_event)


class FeedbackLabel(QLabel):
    """ A temporary label for showing user feedback messages. """
    def __init__(self, message, type_="info", parent=None):
        super().__init__(message, parent)
        colors = {
            'success': ("background-color: #28a745; color: white;"),
            'error': ("background-color: #dc3545; color: white;"),
            'warning': ("background-color: #ffc107; color: black;"),
            'info': ("background-color: #17a2b8; color: white;")
        }
        style = colors.get(type_, colors['info'])
        self.setStyleSheet(f"""
            QLabel {{
                padding: 8px 20px;
                border-radius: 8px;
                {style}
                border: 1px solid rgba(0, 0, 0, 0.1);
            }}
        """)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        font = QFont()
        font.setBold(True)
        self.setFont(font)
        self.raise_()
        self.adjustSize()


class UIManager(QMainWindow):
    """ Main application window and UI manager. """
    def __init__(self, history_manager):
        super().__init__()
        if history_manager is None:
            raise ValueError("UIManager requires a valid history_manager instance.")
        self.history_manager = history_manager
        self.window_pinned = False
        self._force_quit = False
        self.clip_frames = []
        self.settings = {'theme': 'dark', 'window_pinned': False}
        self.empty_label = None
        self.no_results_label = None
        self._feedback_timer = None
        self._feedback_label = None

        self.setWindowTitle("CopyClip")
        self.setMinimumSize(350, 400)
        self.resize(380, 550)

        self.load_settings()
        self.create_gui()
        self.apply_loaded_window_state()
        self.setup_shortcuts() 
        self.hide()

        self.clipboard_timer = QTimer(self)
        self.clipboard_timer.timeout.connect(self.check_clipboard_updates)
        self.clipboard_timer.start(1000)


    def setup_shortcuts(self):
        """ Sets up global shortcuts for the window. """
        shortcut_esc = QShortcut(QKeySequence("Esc"), self)
        shortcut_esc.activated.connect(self.hide_clipboard)

        shortcut_terminate = QShortcut(QKeySequence("Ctrl+Esc"), self)
        shortcut_terminate.activated.connect(self.terminate_application)


    @pyqtSlot()
    def terminate_application(self):
        """ Terminates the application if the window is visible (triggered by Ctrl+Esc). """
        if self.isVisible():
            print("Ctrl+Esc pressed while visible, initiating termination.")
            self._force_quit = True 
            QApplication.instance().quit() 


    def create_gui(self):
        """ Creates the main graphical user interface elements. """
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)

        search_frame = QFrame()
        search_layout = QHBoxLayout(search_frame)
        search_layout.setContentsMargins(0, 0, 0, 0)
        search_layout.setSpacing(5)

        self.search_entry = QLineEdit()
        self.search_entry.setPlaceholderText("Search clips...")
        self.search_entry.textChanged.connect(self.filter_clips)
        self.search_entry.setObjectName("searchEntry")

        clear_button = QPushButton("×")
        clear_button.setFixedSize(28, 28)
        clear_button.setToolTip("Clear search")
        clear_button.setObjectName("clearSearchButton")
        clear_button.clicked.connect(self.clear_search)

        search_layout.addWidget(self.search_entry)
        search_layout.addWidget(clear_button)

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

        instructions_label = QLabel("Left Click: Copy | Ctrl+Click: Toggle Pin")
        instructions_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        instructions_label.setObjectName("instructionsLabel")
        font_instr = QFont()
        font_instr.setPointSize(9)
        instructions_label.setFont(font_instr)

        button_frame = QFrame()
        button_layout = QHBoxLayout(button_frame)
        button_layout.setContentsMargins(0, 0, 0, 0)
        button_layout.setSpacing(10)

        clear_history_btn = QPushButton("Clear History")
        clear_history_btn.setToolTip("Remove all non-pinned items")
        clear_history_btn.clicked.connect(self.clear_history)

        settings_btn = QPushButton("Settings")
        settings_btn.setToolTip("Open application settings")
        settings_btn.clicked.connect(self.show_settings)

        self.pin_button = QPushButton("Pin Window")
        self.pin_button.setToolTip("Keep window on top and open")
        self.pin_button.setCheckable(True)
        self.pin_button.toggled.connect(self.toggle_pin)

        button_layout.addStretch(1)
        button_layout.addWidget(clear_history_btn)
        button_layout.addWidget(settings_btn)
        button_layout.addWidget(self.pin_button)
        button_layout.addStretch(1)

        main_layout.addWidget(search_frame)
        main_layout.addWidget(self.clips_scroll_area, 1)
        main_layout.addWidget(instructions_label)
        main_layout.addWidget(button_frame)

        self.apply_theme(self.settings.get('theme', 'dark'))


    def apply_loaded_window_state(self):
        """Applies window state settings after GUI creation."""
        is_pinned = self.settings.get('window_pinned', False)
        self.window_pinned = is_pinned
        self.pin_button.setChecked(is_pinned)
        self.pin_button.setText("Unpin Window" if is_pinned else "Pin Window")
        self.setWindowFlag(Qt.WindowType.WindowStaysOnTopHint, is_pinned)


    @pyqtSlot(str)
    def handle_copy_clicked(self, content):
        """ Slot to handle copy signal from ClipFrame (receives original content). """
        self.copy_to_clipboard(content)


    @pyqtSlot(str)
    def handle_pin_clicked(self, content):
        """ Slot to handle pin toggle signal from ClipFrame (receives original content). """
        self.toggle_clip_pin(content)


    def update_clips_display(self):
        """ Clears and redraws the list of clips in the UI. """
        items_to_remove = []
        widgets_to_delete = []
        for i in range(self.clips_layout.count()):
            item = self.clips_layout.itemAt(i)
            widget = item.widget()
            if isinstance(widget, ClipFrame):
                items_to_remove.append(item)
                widgets_to_delete.append(widget)
                try:
                    widget.copy_clicked.disconnect(self.handle_copy_clicked)

                except (TypeError, AttributeError): pass
                try:
                    widget.pin_clicked.disconnect(self.handle_pin_clicked)

                except (TypeError, AttributeError): pass

        for item in items_to_remove:
            self.clips_layout.removeItem(item)

        for widget in widgets_to_delete:
            widget.setParent(None)
            widget.deleteLater()

        self.clip_frames.clear()

        try:
            history_items = self.history_manager.get_history()
        except Exception as e:
            print(f"Error getting history: {e}")
            self.show_feedback("Error loading history", "error")
            history_items = []

        self.empty_label.hide()
        self.no_results_label.hide()

        insertion_index = 0
        if history_items:
            for item in history_items:
                try:
                    clip_frame = ClipFrame(item)
                    clip_frame.copy_clicked.connect(self.handle_copy_clicked)
                    clip_frame.pin_clicked.connect(self.handle_pin_clicked)
                    self.clips_layout.insertWidget(insertion_index, clip_frame)
                    self.clip_frames.append(clip_frame)
                    insertion_index += 1

                except Exception as e:
                    print(f"Error creating ClipFrame for item {item.get('timestamp', '')}: {e}")
        else:
            self.empty_label.show()

        self.filter_clips()


    def check_clipboard_updates(self):
        """ Checks the clipboard for new content via the history manager. """
        try:
            content = self.history_manager.clipboard_manager.check_for_new_content()
            if content:
                added = self.history_manager.add_to_history(content)
                if added:
                    self.update_clips_display()

        except AttributeError:
            print("Error: history_manager or clipboard_manager not properly configured.")
            self.clipboard_timer.stop()

        except Exception as e:
            print(f"Error checking clipboard: {e}")


    def copy_to_clipboard(self, content):
        """ Copies the given (original) content to the system clipboard. """
        if not content:
            self.show_feedback("Nothing to copy", "warning")
            return False
        try:
            success = self.history_manager.clipboard_manager.set_clipboard_content(content)
            if success:
                self.show_feedback("Copied successfully!", "success")
                if not self.window_pinned:
                    QTimer.singleShot(800, self.hide_clipboard)
                return True
            else:
                self.show_feedback("Failed to copy content", "error")
                return False

        except AttributeError:
            print("Error: history_manager or clipboard_manager not properly configured.")
            self.show_feedback("Clipboard functionality error", "error")
            return False

        except Exception as e:
            print(f"Error copying to clipboard: {e}")
            self.show_feedback(f"Error copying: {str(e)}", "error")
            return False


    def show_feedback(self, message, type_="info", duration=2000):
        """ Displays a temporary feedback message at the bottom of the window. """
        if self._feedback_label:
            self._feedback_label.deleteLater()
            if self._feedback_timer:
                self._feedback_timer.stop()

        self._feedback_label = FeedbackLabel(message, type_, self.centralWidget())
        label_size = self._feedback_label.sizeHint()
        x = (self.width() - label_size.width()) // 2
        y = self.height() - label_size.height() - 15
        x = max(0, x)
        y = max(0, y)
        self._feedback_label.move(x, y)
        self._feedback_label.show()
        self._feedback_label.raise_()

        if self._feedback_timer:
            self._feedback_timer.stop()
        else:
            self._feedback_timer = QTimer(self)
            self._feedback_timer.setSingleShot(True)
            self._feedback_timer.timeout.connect(self.hide_feedback)

        self._feedback_timer.start(duration)


    def hide_feedback(self):
        """ Hides the feedback label. """
        if self._feedback_label:
            self._feedback_label.deleteLater()
            self._feedback_label = None
        if self._feedback_timer:
            self._feedback_timer.stop()


    def toggle_clip_pin(self, content):
        """ Toggles the pinned state of a specific clipboard item (uses original content). """
        try:
            is_pinned = self.history_manager.toggle_pin(content)
            self.update_clips_display()
            status = "pinned" if is_pinned else "unpinned"
            self.show_feedback(f"Item {status}", "info", duration=1500)
        except Exception as e:
            print(f"Error toggling pin for item: {e}")
            self.show_feedback("Error updating pin status", "error")


    def filter_clips(self):
        """ Shows or hides ClipFrames and status labels based on the search query. """
        search_text = self.search_entry.text().lower().strip()
        has_visible_items = False
        self.empty_label.hide()
        self.no_results_label.hide()

        history_has_items = False
        for frame in self.clip_frames:
            history_has_items = True
            content_match = search_text in frame.original_content.lower()
            frame.setVisible(content_match)
            if content_match:
                has_visible_items = True

        is_searching = bool(search_text)

        if not history_has_items:
            self.empty_label.show()
        elif is_searching and not has_visible_items:
            self.no_results_label.show()


    def clear_search(self):
        """ Clears the search input field. """
        self.search_entry.clear()


    def clear_history(self):
        """ Clears non-pinned items from the history. """
        reply = QMessageBox.question(self, 'Confirm Clear',
                                    "Are you sure you want to clear all non-pinned items?",
                                    QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                                    QMessageBox.StandardButton.No)
        if reply == QMessageBox.StandardButton.Yes:
            try:
                self.history_manager.clear_history()
                self.update_clips_display()
                self.show_feedback("History cleared (pinned items kept)", "info")
            except Exception as e:
                print(f"Error clearing history: {e}")
                self.show_feedback("Error clearing history", "error")


    @pyqtSlot(bool)
    def toggle_pin(self, checked):
        """ Toggles the 'always on top' state of the window. """
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
            self.settings['window_pinned'] = self.window_pinned
            self.save_settings()
        
        except Exception as e:
            print(f"Error in toggle_pin: {e}")

            self.pin_button.setChecked(not checked)
            flags = self.windowFlags()
            if not checked:
                 self.setWindowFlags(flags | Qt.WindowType.WindowStaysOnTopHint)
                 self.pin_button.setText("Unpin Window")
            else:
                 self.setWindowFlags(flags & ~Qt.WindowType.WindowStaysOnTopHint)
                 self.pin_button.setText("Pin Window")
            self.show()


    def show_clipboard(self):
        """ Shows the main window, updates clips, and brings it to front. """
        self.update_clips_display()
        self.show()
        self.raise_()
        self.activateWindow()
        self.clear_search()
        self.search_entry.setFocus()


    def hide_clipboard(self):
        """ Hides the main window if it's not pinned. """
        if not self.window_pinned:
            self.hide()
            self.clear_search()


    def closeEvent(self, event):
        """ Overrides the default close event. """
        # First check if a forced quit (Ctrl+Esc) was requested
        if self._force_quit:
            print("Force quit flag detected, accepting close event.")
            event.accept()
            return 

        if self.window_pinned:
            event.ignore()
            self.show_feedback("Window is pinned. Unpin to close or hide.", "warning", duration=2500)
        else:
            event.ignore()
            self.hide_clipboard()


    def get_settings_path(self):
        """ Gets the path for the settings.json file. """
        try:
            history_dir = os.path.dirname(self.history_manager.history_file)
            os.makedirs(history_dir, exist_ok=True)
            return os.path.join(history_dir, 'settings.json')
        
        except AttributeError:
            print("Warning: history_manager does not have history_file attribute. Using current directory for settings.")
            return 'settings.json'
        
        except Exception as e:
            print(f"Error determining settings path: {e}")
            return 'settings.json'


    def load_settings(self):
        """ Loads settings from settings.json. """
        settings_file = self.get_settings_path()
        default_settings = {'theme': 'dark', 'window_pinned': False}
        try:
            if os.path.exists(settings_file):
                with open(settings_file, 'r', encoding='utf-8') as f:
                    loaded_settings = json.load(f)
                    self.settings = default_settings | loaded_settings
            else:
                 self.settings = default_settings
                 print("Settings file not found, using defaults.")
        
        except json.JSONDecodeError:
            print(f"Error: Settings file ({settings_file}) is corrupted. Using default settings.")
            self.settings = default_settings
            try:
                corrupted_path = settings_file + ".corrupted_" + datetime.now().strftime("%Y%m%d%H%M%S")
                os.rename(settings_file, corrupted_path)
                print(f"Corrupted settings file backed up to {corrupted_path}")
            
            except OSError as backup_err:
                print(f"Could not back up corrupted settings file: {backup_err}")
        
        except Exception as e:
            print(f"Error loading settings from {settings_file}: {e}")
            self.settings = default_settings


    def save_settings(self):
        """ Saves current settings to settings.json. """
        settings_file = self.get_settings_path()
        try:
            os.makedirs(os.path.dirname(settings_file), exist_ok=True)
            with open(settings_file, 'w', encoding='utf-8') as f:
                json.dump(self.settings, f, indent=4)
        
        except Exception as e:
            print(f"Error saving settings to {settings_file}: {e}")
            self.show_feedback("Error saving settings", "error")


    def apply_theme(self, theme='dark'):
        """ Applies the selected visual theme (stylesheet). """
        scrollbar_style_hidden = """
            QScrollArea QScrollBar:vertical {
                border: none; background: transparent; width: 0px; margin: 0px;
            }
            QScrollArea QScrollBar::handle:vertical {
                background: transparent; min-height: 0px; border-radius: 0px;
            }
            QScrollArea QScrollBar::add-line:vertical, QScrollArea QScrollBar::sub-line:vertical {
                border: none; background: none; height: 0px;
            }
            QScrollArea QScrollBar::add-page:vertical, QScrollArea QScrollBar::sub-page:vertical {
                background: none;
            }
            QScrollArea QScrollBar:horizontal {
                 border: none; background: transparent; height: 0px; margin: 0px;
            }
            QScrollArea QScrollBar::handle:horizontal {
                 background: transparent; min-width: 0px; border-radius: 0px;
            }
        """

        toggle_label_style = """
            QLabel#toggleContentLabel {
                color: #007bff; /* Link-like color */
                background-color: transparent;
                padding: 2px 0px;
                font-size: 8pt;
            }
            QLabel#toggleContentLabel:hover {
                text-decoration: underline;
            }
        """

        dark_stylesheet = f"""
            QMainWindow, QWidget {{
                background-color: #1e1e1e; color: #e0e0e0;
            }}
            QPushButton {{
                background-color: #007bff; color: #ffffff; border: none;
                padding: 6px 12px; border-radius: 4px; font-weight: normal;
            }}
            QPushButton:hover {{ background-color: #0056b3; }}
            QPushButton:pressed {{ background-color: #004085; }}
            QPushButton#clearSearchButton {{
                background-color: #4e4e4e; color: #cccccc; font-weight: bold;
                padding: 0px; border-radius: 14px;
            }}
            QPushButton#clearSearchButton:hover {{ background-color: #666666; }}
            QLineEdit {{
                background-color: #2a2a2a; color: #e0e0e0;
                border: 1px solid #3d3d3d; border-radius: 4px; padding: 5px;
            }}
            QScrollArea {{ background-color: transparent; border: none; }}
            QWidget#clips_content_widget {{ background-color: transparent; }}
            {scrollbar_style_hidden}
            QComboBox {{
                background-color: #2a2a2a; border: 1px solid #3d3d3d;
                border-radius: 4px; padding: 5px;
            }}
            QComboBox QAbstractItemView {{
                background-color: #2a2a2a; color: white;
                selection-background-color: #007bff; border: 1px solid #3d3d3d;
            }}
            QLabel {{ color: #e0e0e0; }}
            QLabel#instructionsLabel {{ color: #aaaaaa; font-size: 9pt; }}
            QLabel#emptyHistoryLabel, QLabel#noResultsLabel {{ color: #aaaaaa; padding: 20px; }}
            QToolTip {{
                background-color: #333333; color: #ffffff;
                border: 1px solid #444444; border-radius: 3px; padding: 4px;
            }}
            QFrame#clipFrame {{
                background-color: #252526; border-radius: 5px;
                margin: 0px 2px 3px 2px; border: 1px solid #3d3d3d;
            }}
            QFrame#clipFrame:hover {{
                background-color: #383838; border: 1px solid #505050;
            }}
            QFrame#clipFrame QLabel#timeLabel {{
                color: #aaaaaa; background-color: transparent; padding: 2px 0px;
            }}
            QFrame#clipFrame QLabel#contentLabel {{
                color: #e0e0e0;
                background-color: #2c2c2e;
                padding: 3px 5px;
                border-radius: 3px;
            }}
            QFrame#clipFrame:hover QLabel#contentLabel {{
                background-color: #404040;
            }}
            QFrame#clipFrame QLabel#pinIndicator {{
                color: #ffcc00; background-color: transparent;
                font-weight: bold; padding-right: 5px;
            }}
            QDialog {{ background-color: #1e1e1e; }}
            {toggle_label_style.replace("#007bff", "#58a6ff")}
        """

        light_stylesheet = f"""
            QMainWindow, QWidget {{ background-color: #f8f9fa; color: #212529; }}
            QPushButton {{
                background-color: #007bff; color: #ffffff; border: none;
                padding: 6px 12px; border-radius: 4px; font-weight: normal;
            }}
            QPushButton:hover {{ background-color: #0056b3; }}
            QPushButton:pressed {{ background-color: #004085; }}
            QPushButton#clearSearchButton {{
                background-color: #d0d0d0; color: #333333; font-weight: bold;
                padding: 0px; border-radius: 14px;
            }}
            QPushButton#clearSearchButton:hover {{ background-color: #bbbbbb; }}
            QLineEdit {{
                background-color: #ffffff; color: #212529;
                border: 1px solid #ced4da; border-radius: 4px; padding: 5px;
            }}
            QScrollArea {{ background-color: transparent; border: none; }}
            QWidget#clips_content_widget {{ background-color: transparent; }}
            {scrollbar_style_hidden}
            QComboBox {{
                background-color: #ffffff; border: 1px solid #ced4da;
                border-radius: 4px; padding: 5px; color: #212529;
            }}
            QComboBox QAbstractItemView {{
                background-color: #ffffff; color: #212529;
                selection-background-color: #007bff; selection-color: #ffffff;
                border: 1px solid #ced4da;
            }}
            QLabel {{ color: #212529; }}
            QLabel#instructionsLabel {{ color: #6c757d; font-size: 9pt; }}
            QLabel#emptyHistoryLabel, QLabel#noResultsLabel {{ color: #6c757d; padding: 20px; }}
            QToolTip {{
                background-color: #ffffff; color: #000000;
                border: 1px solid #ced4da; border-radius: 3px; padding: 4px;
            }}
            QFrame#clipFrame {{
                background-color: #ffffff; border: 1px solid #dee2e6;
                border-radius: 5px; margin: 0px 2px 3px 2px;
            }}
            QFrame#clipFrame:hover {{
                background-color: #f1f3f5; border: 1px solid #adb5bd;
            }}
            QFrame#clipFrame QLabel#timeLabel {{
                color: #6c757d; background-color: transparent; padding: 2px 0px;
            }}
            QFrame#clipFrame QLabel#contentLabel {{
                color: #212529;
                background-color: #f0f0f0;
                padding: 3px 5px;
                border-radius: 3px;
            }}
            QFrame#clipFrame:hover QLabel#contentLabel {{
                background-color: #e9ecef;
            }}
            QFrame#clipFrame QLabel#pinIndicator {{
                color: #dc3545; background-color: transparent;
                font-weight: bold; padding-right: 5px;
            }}
            QDialog {{ background-color: #f8f9fa; }}
            {toggle_label_style}
        """

        valid_theme = theme
        if theme == 'dark':
            self.setStyleSheet(dark_stylesheet)
        elif theme == 'light':
            self.setStyleSheet(light_stylesheet)
        elif theme == 'system':
            self.setStyleSheet("") # Reset first
            system_essential = f"""
                 {scrollbar_style_hidden}
                 {toggle_label_style}
                 QFrame#clipFrame QLabel#contentLabel {{
                     padding: 3px 5px;
                     border-radius: 3px;
                 }}
                 QFrame#clipFrame QLabel#timeLabel {{
                     padding: 2px 0px;
                 }}
                  QFrame#clipFrame QLabel#pinIndicator {{
                    font-weight: bold; padding-right: 5px;
                 }}
             """
            current_style = self.styleSheet()
            self.setStyleSheet(current_style + system_essential)

        else:
            print(f"Warning: Unknown theme '{theme}'. Falling back to dark theme.")
            self.setStyleSheet(dark_stylesheet)
            valid_theme = 'dark'

        self.settings['theme'] = valid_theme
        self.save_settings()


    def show_settings(self):
        """ Displays a dialog for changing application settings. """
        settings_dialog = QDialog(self)
        settings_dialog.setWindowTitle("Settings")
        settings_dialog.setMinimumWidth(300)
        settings_dialog.setModal(True)

        layout = QVBoxLayout(settings_dialog)
        layout.setContentsMargins(15, 15, 15, 15)
        layout.setSpacing(10)

        theme_label = QLabel("Theme:")
        layout.addWidget(theme_label)

        theme_combo = QComboBox()
        theme_combo.setObjectName("themeComboBox")
        theme_combo.addItems(["dark", "light", "system"])
        current_theme = self.settings.get('theme', 'dark')
        theme_combo.setCurrentText(current_theme)
        theme_combo.currentTextChanged.connect(self.apply_theme)
        layout.addWidget(theme_combo)

        layout.addStretch(1)

        button_box = QHBoxLayout()
        button_box.addStretch(1)
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(settings_dialog.accept)
        button_box.addWidget(close_btn)
        layout.addLayout(button_box)

        try:
            main_geo = self.geometry()
            dialog_size = settings_dialog.sizeHint()
            x = main_geo.x() + (main_geo.width() - dialog_size.width()) // 2
            y = main_geo.y() + (main_geo.height() - dialog_size.height()) // 2
            settings_dialog.move(max(0, x), max(0, y))
        
        except Exception as e_pos:
            print(f"Could not position settings dialog relative to main window: {e_pos}")


        settings_dialog.exec()