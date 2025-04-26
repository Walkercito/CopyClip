# ui.py
import sys
import os
import json
from datetime import datetime
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                           QHBoxLayout, QFrame, QLabel, QPushButton, 
                           QLineEdit, QScrollArea, QMessageBox, QComboBox)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QSize
from PyQt6.QtGui import QFont, QIcon, QKeySequence, QShortcut

class ClipFrame(QFrame):
    """Custom widget to represent each clipboard item"""
    def __init__(self, item, parent=None):
        super().__init__(parent)
        self.content = item['content']
        self.pinned = item['pinned']
        self.timestamp = item['timestamp']
        
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setFrameShadow(QFrame.Shadow.Raised)
        
        # Styles
        self.setObjectName("clipFrame")
        self.setStyleSheet("""
            #clipFrame {
                background-color: #2b2b2b;
                border-radius: 5px;
                margin: 3px;
            }
        """)
        
        layout = QHBoxLayout(self)

        content_preview = (self.content[:50] + "...") if len(self.content) > 50 else self.content
        label = QLabel(f"{self.timestamp}\n{content_preview}")
        label.setWordWrap(True)
        label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        
        pin_text = "Unpin" if self.pinned else "Pin"
        self.pin_btn = QPushButton(pin_text)
        self.pin_btn.setFixedWidth(60)
        
        copy_btn = QPushButton("Copy")
        copy_btn.setFixedWidth(60)
        
        layout.addWidget(label, 1)
        layout.addWidget(self.pin_btn)
        layout.addWidget(copy_btn)
        
        self.setLayout(layout)

class FeedbackLabel(QLabel):
    """Custom label to display feedback messages"""
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
            padding: 5px 10px;
            border-radius: 8px;
            {style}
        """)
        
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        font = QFont()
        font.setBold(True)
        self.setFont(font)

class UIManager(QMainWindow):
    def __init__(self, history_manager):
        super().__init__()
        self.history_manager = history_manager
        self.window_pinned = False
        self.clip_frames = []
        self.settings = {}
        
        self.setWindowTitle("CopyClip")
        self.setGeometry(100, 100, 300, 500)
        self.load_settings()
        self.create_gui()
        self.hide()

        self.clipboard_timer = QTimer(self)
        self.clipboard_timer.timeout.connect(self.check_clipboard_updates)
        self.clipboard_timer.start(1000)
        
        shortcut = QShortcut(QKeySequence("Esc"), self)
        shortcut.activated.connect(self.hide_clipboard)


    def create_gui(self):
        central_widget = QWidget()
        main_layout = QVBoxLayout(central_widget)

        search_frame = QFrame()
        search_layout = QHBoxLayout(search_frame)
        
        self.search_entry = QLineEdit()
        self.search_entry.setPlaceholderText("Search clips...")
        self.search_entry.textChanged.connect(self.filter_clips)
        
        clear_button = QPushButton("×")
        clear_button.setFixedSize(32, 32)
        clear_button.clicked.connect(self.clear_search)
        
        search_layout.addWidget(self.search_entry)
        search_layout.addWidget(clear_button)
        
        self.clips_scroll_area = QScrollArea()
        self.clips_scroll_area.setWidgetResizable(True)
        self.clips_content = QWidget()
        self.clips_layout = QVBoxLayout(self.clips_content)
        self.clips_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        self.clips_scroll_area.setWidget(self.clips_content)
        
        button_frame = QFrame()
        button_layout = QHBoxLayout(button_frame)
        
        clear_btn = QPushButton("Clear All")
        clear_btn.clicked.connect(self.clear_history)
        
        settings_btn = QPushButton("Settings")
        settings_btn.clicked.connect(self.show_settings)
        
        self.pin_button = QPushButton("Pin Window")
        self.pin_button.clicked.connect(self.toggle_pin)
        
        button_layout.addWidget(clear_btn)
        button_layout.addWidget(settings_btn)
        button_layout.addWidget(self.pin_button)
        
        main_layout.addWidget(search_frame)
        main_layout.addWidget(self.clips_scroll_area, 1)
        main_layout.addWidget(button_frame)
        
        self.setCentralWidget(central_widget)
        self.apply_theme()
    

    def update_clips_display(self):
        # Clear existing widgets
        for i in reversed(range(self.clips_layout.count())): 
            self.clips_layout.itemAt(i).widget().setParent(None)
        
        self.clip_frames = []
        
        # Add clips from history
        for item in self.history_manager.get_history():
            clip_frame = ClipFrame(item)
            clip_frame.pin_btn.clicked.connect(lambda _, content=item['content']: self.toggle_clip_pin(content))
            
            copy_btn = clip_frame.findChild(QPushButton, "", Qt.FindChildOption.FindChildrenRecursively)
            if copy_btn and copy_btn.text() == "Copy":
                copy_btn.clicked.connect(lambda _, content=item['content']: self.copy_to_clipboard(content))
            
            self.clips_layout.addWidget(clip_frame)
            self.clip_frames.append(clip_frame)


    def check_clipboard_updates(self):
        content = self.history_manager.clipboard_manager.check_for_new_content()
        if content:
            self.history_manager.add_to_history(content)
            self.update_clips_display()


    def copy_to_clipboard(self, content):
        """Copy content to clipboard with error handling and feedback"""
        if not content:
            self.show_feedback("Nothing to copy", "warning")
            return False
        
        try:
            success = self.history_manager.clipboard_manager.set_clipboard_content(content)
            
            if success:
                self.show_feedback("Copied successfully!", "success")
                QTimer.singleShot(1000, self.hide_clipboard)
                return True
            else:
                self.show_feedback("Failed to copy content", "error")
                return False
                
        except Exception as e:
            print(f"Error copying to clipboard: {e}")
            self.show_feedback(f"Error copying: {str(e)}", "error")
            return False


    def show_feedback(self, message, type_="info"):
        """Show feedback message to user"""
        feedback = FeedbackLabel(message, type_, self)

        feedback.move(
            (self.width() - feedback.width()) // 2,
            self.height() - 60
        )
        
        feedback.show()

        QTimer.singleShot(2000, feedback.deleteLater)


    def toggle_clip_pin(self, content):
        self.history_manager.toggle_pin(content)
        self.update_clips_display()


    def filter_clips(self):
        search_text = self.search_entry.text().lower()
        for frame in self.clip_frames:
            if search_text in frame.content.lower() or search_text in frame.timestamp.lower():
                frame.show()
            else:
                frame.hide()


    def clear_search(self):
        self.search_entry.clear()
        self.filter_clips()


    def clear_history(self):
        self.history_manager.clear_history()
        self.update_clips_display()


    def toggle_pin(self):
        """Toggle window pin state and apply restrictions"""
        try:
            self.window_pinned = not self.window_pinned
            
            self.setWindowFlag(Qt.WindowType.WindowStaysOnTopHint, self.window_pinned)
            self.show()
            
            if self.window_pinned:
                self.setFixedSize(self.size())
                self.pin_button.setText("Unpin Window")
            else:
                self.setMinimumSize(200, 300)
                self.setMaximumSize(16777215, 16777215)  # Maximum value for QWidget
                self.pin_button.setText("Pin Window")
            
            self.settings['window_pinned'] = self.window_pinned
            self.save_settings()
            
        except Exception as e:
            print(f"Error in toggle_pin: {e}")
    

    def show_clipboard(self):
        """Show clipboard window"""
        self.update_clips_display()
        self.show()
        self.activateWindow()
        self.raise_()
        self.clear_search()
        self.search_entry.setFocus()


    def hide_clipboard(self):
        """Hide clipboard window instead of closing the application"""
        self.hide()
        self.clear_search()


    def closeEvent(self, event):
        """Capture close event to hide instead of close"""
        if not self.window_pinned:
            event.ignore()
            self.hide_clipboard()
        else:
            event.ignore()  # Don't allow closing when pinned


    def load_settings(self):
        """Load settings with error handling"""
        self.settings = {}
        try:
            settings_dir = os.path.dirname(self.history_manager.history_file)
            settings_file = os.path.join(settings_dir, 'settings.json')
            os.makedirs(settings_dir, exist_ok=True)
            
            if os.path.exists(settings_file):
                try:
                    with open(settings_file, 'r', encoding='utf-8') as f:
                        self.settings = json.load(f)
                except json.JSONDecodeError:
                    print("Settings file is corrupted, using defaults")
                    corrupted_file = f"{settings_file}.corrupted"
                    os.rename(settings_file, corrupted_file)
                except Exception as e:
                    print(f"Error loading settings: {e}")
            
            self.apply_settings()
            
        except Exception as e:
            print(f"Error in load_settings: {e}")
            self.settings = {
                'theme': 'dark',
                'window_pinned': False
            }


    def apply_settings(self):
        """Apply settings with default values if needed"""
        try:
            theme = self.settings.get('theme', 'dark')
            self.apply_theme(theme)
            
            if self.settings.get('window_pinned', False):
                self.window_pinned = True
                self.toggle_pin()
                
        except Exception as e:
            print(f"Error applying settings: {e}")


    def apply_theme(self, theme='dark'):
        """Apply visual theme to the application"""
        if theme == 'dark':
            self.setStyleSheet("""
                QMainWindow, QWidget {
                    background-color: #1e1e1e;
                    color: #ffffff;
                }
                QPushButton {
                    background-color: #0d6efd;
                    color: white;
                    border: none;
                    padding: 5px;
                    border-radius: 3px;
                }
                QPushButton:hover {
                    background-color: #0b5ed7;
                }
                QLineEdit {
                    background-color: #2d2d2d;
                    color: white;
                    border: 1px solid #3d3d3d;
                    border-radius: 3px;
                    padding: 5px;
                }
                QScrollArea, QComboBox {
                    background-color: #2d2d2d;
                    border: 1px solid #3d3d3d;
                    border-radius: 3px;
                }
                QLabel {
                    color: #ffffff;
                }
                QComboBox {
                    padding: 5px;
                }
                QComboBox QAbstractItemView {
                    background-color: #2d2d2d;
                    color: white;
                }
            """)
        elif theme == 'light':
            self.setStyleSheet("""
                QMainWindow, QWidget {
                    background-color: #f8f9fa;
                    color: #000000;
                }
                QPushButton {
                    background-color: #0d6efd;
                    color: white;
                    border: none;
                    padding: 5px;
                    border-radius: 3px;
                }
                QPushButton:hover {
                    background-color: #0b5ed7;
                }
                QLineEdit {
                    background-color: white;
                    color: black;
                    border: 1px solid #ced4da;
                    border-radius: 3px;
                    padding: 5px;
                }
                QScrollArea, QComboBox {
                    background-color: white;
                    border: 1px solid #ced4da;
                    border-radius: 3px;
                }
                QLabel {
                    color: #000000;
                }
                QComboBox {
                    padding: 5px;
                }
                QComboBox QAbstractItemView {
                    background-color: white;
                    color: black;
                }
            """)
        elif theme == 'system':
            self.setStyleSheet("")  # Use system default style
        
        self.settings['theme'] = theme
        self.save_settings()


    def save_settings(self):
        settings_file = os.path.join(
            os.path.dirname(self.history_manager.history_file),
            'settings.json'
        )
        try:
            with open(settings_file, 'w') as f:
                json.dump(self.settings, f)
        except Exception as e:
            print(f"Error saving settings: {e}")


    def show_settings(self):
        """Show settings window"""
        from PyQt6.QtWidgets import QDialog, QVBoxLayout, QLabel, QComboBox, QPushButton
        
        settings_dialog = QDialog(self)
        settings_dialog.setWindowTitle("Settings")
        settings_dialog.setFixedSize(300, 200)
        
        layout = QVBoxLayout(settings_dialog)
        
        # Theme
        theme_label = QLabel("Theme:")
        layout.addWidget(theme_label)
        
        theme_combo = QComboBox()
        theme_combo.addItems(["light", "dark", "system"])
        theme_combo.setCurrentText(self.settings.get('theme', 'dark'))
        theme_combo.currentTextChanged.connect(self.apply_theme)
        layout.addWidget(theme_combo)
        
        # Close button
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(settings_dialog.accept)
        layout.addWidget(close_btn, alignmvent=Qt.AlignmentFlag.AlignBottom)
        
        # Center dialog
        settings_dialog.move(
            self.x() + (self.width() - settings_dialog.width()) // 2,
            self.y() + (self.height() - settings_dialog.height()) // 2
        )
        
        settings_dialog.exec()