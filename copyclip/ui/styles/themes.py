"""Theme management and stylesheets for the application."""

from copyclip.utils.constants import Theme


class ThemeManager:
    """Manages application themes and stylesheets."""

    # Common styles used across themes
    SCROLLBAR_HIDDEN = """
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

    TOGGLE_LABEL = """
        QLabel#toggleContentLabel {{
            color: {color};
            background-color: transparent;
            padding: 2px 0px;
            font-size: 8pt;
        }}
        QLabel#toggleContentLabel:hover {{
            text-decoration: underline;
        }}
    """

    @classmethod
    def get_dark_theme(cls) -> str:
        """Get the dark theme stylesheet.

        Returns:
            Dark theme stylesheet string
        """
        return f"""
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
            {cls.SCROLLBAR_HIDDEN}
            QComboBox {{
                background-color: #2a2a2a; border: 1px solid #3d3d3d;
                border-radius: 4px; padding: 5px; color: #e0e0e0;
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
            QDialog {{ background-color: #1e1e1e; color: #e0e0e0; }}
            QRadioButton {{ color: #e0e0e0; }}
            {cls.TOGGLE_LABEL.format(color="#58a6ff")}
        """

    @classmethod
    def get_light_theme(cls) -> str:
        """Get the light theme stylesheet.

        Returns:
            Light theme stylesheet string
        """
        return f"""
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
            {cls.SCROLLBAR_HIDDEN}
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
            QDialog {{ background-color: #f8f9fa; color: #212529; }}
            QRadioButton {{ color: #212529; }}
            {cls.TOGGLE_LABEL.format(color="#007bff")}
        """

    @classmethod
    def get_system_theme(cls) -> str:
        """Get the system theme stylesheet (minimal styling).

        Returns:
            System theme stylesheet string
        """
        return (
            cls.SCROLLBAR_HIDDEN
            + cls.TOGGLE_LABEL.format(color="#007bff")
            + """
            QFrame#clipFrame QLabel#contentLabel {
                padding: 3px 5px;
                border-radius: 3px;
            }
            QFrame#clipFrame QLabel#timeLabel {
                padding: 2px 0px;
            }
            QFrame#clipFrame QLabel#pinIndicator {
                font-weight: bold; padding-right: 5px;
            }
        """
        )

    @classmethod
    def get_theme_stylesheet(cls, theme: str) -> str:
        """Get stylesheet for the specified theme.

        Args:
            theme: Theme name (dark, light, or system)

        Returns:
            Stylesheet string for the theme
        """
        if theme == Theme.DARK:
            return cls.get_dark_theme()
        elif theme == Theme.LIGHT:
            return cls.get_light_theme()
        elif theme == Theme.SYSTEM:
            return cls.get_system_theme()
        else:
            print(f"Warning: Unknown theme '{theme}', using dark theme")
            return cls.get_dark_theme()
