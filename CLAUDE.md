# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CopyClip is a modern Linux clipboard manager built with PyQt6, inspired by Windows 10's clipboard features. It monitors clipboard changes, stores history persistently in JSON, and provides a GUI for managing clipboard items with features like pinning, searching, and themes.

**Key Features:**
- Configurable global hotkeys with first-run setup
- X11 and Wayland support (X11: full global hotkeys, Wayland: manual setup)
- Multiple theme support (dark, light, system)
- Clipboard history with search and pinning
- Modular, well-typed codebase following DRY principles

## Development Commands

### Running the Application
```bash
# Run the full application with hotkeys
uv run main.py

# Alternative: Show UI directly without hotkeys
uv run bin/show_ui.py

# Manual UI launcher (for system hotkey integration)
./bin/copyclip-show-ui
```

### Installing Dependencies
```bash
# Using uv (recommended)
uv sync

# System dependencies (required for clipboard operations)
# Debian/Ubuntu:
sudo apt-get install xsel python3-xlib python3-pyqt6

# Fedora:
sudo dnf install xsel python3-xlib python3-pyqt6

# Arch/Manjaro:
sudo pacman -S xsel python-xlib python-pyqt6
```

### Code Quality
```bash
# Run linting and formatting
uv run ruff check --fix .
uv run ruff format .

# Install pre-commit hooks (one time)
uv run pre-commit install

# Run pre-commit on all files manually
uv run pre-commit run --all-files
```

## Architecture

### New Modular Structure

The application has been completely refactored into a well-organized package structure:

```
copyclip/
├── core/                   # Core business logic
│   ├── clipboard.py        # Clipboard operations via xsel
│   ├── history.py          # History management with JSON persistence
│   └── settings.py         # Settings manager (DRY)
├── hotkeys/                # Hotkey system with X11/Wayland support
│   ├── backend_base.py     # Abstract backend interface
│   ├── backend_x11.py      # X11 implementation (Xlib RECORD)
│   ├── backend_wayland.py  # Wayland support (manual setup)
│   ├── config.py           # Hotkey configuration and presets
│   └── manager.py          # Auto-detection and backend selection
├── ui/                     # User interface components
│   ├── dialogs/
│   │   ├── first_run.py    # First-run hotkey selection dialog
│   │   └── settings.py     # Settings dialog
│   ├── styles/
│   │   └── themes.py       # Theme manager with DRY stylesheets
│   ├── widgets/
│   │   ├── clip_frame.py   # Individual clipboard item widget
│   │   ├── clickable_label.py  # Clickable label component
│   │   └── feedback_label.py   # Feedback message label
│   └── main_window.py      # Main application window
└── utils/                  # Utilities and helpers
    ├── constants.py        # Application-wide constants
    └── environment.py      # X11/Wayland detection
```

### Component Details

#### Core Layer (`copyclip/core/`)

**ClipboardManager** (`clipboard.py`)
- Low-level clipboard operations using `xsel` subprocess
- Methods: `get_content()`, `set_content()`, `check_for_changes()`
- Verifies xsel installation with helpful error messages
- Tracks current state to detect changes

**HistoryManager** (`history.py`)
- Persistent clipboard history storage
- Location: `~/.local/share/clipboard-manager/clipboard_history.json`
- Uses `TypedDict` for type safety (`HistoryItem`)
- Implements deduplication (existing items moved to top)
- Methods: `add_item()`, `toggle_pin()`, `clear_history()`, `get_sorted_history()`

**SettingsManager** (`settings.py`)
- Centralized settings management
- Location: `~/.local/share/clipboard-manager/settings.json`
- Methods: `get()`, `set()`, `save()`, `is_first_run()`
- Handles corruption gracefully with backups

#### Hotkey Layer (`copyclip/hotkeys/`)

**HotkeyManager** (`manager.py`)
- Auto-detects X11 vs Wayland via environment detection
- Dynamically loads appropriate backend
- Provides unified interface for hotkey operations
- Methods: `setup_hotkeys()`, `stop_listening()`, `update_hotkey()`, `enable_debug()`

**X11HotkeyBackend** (`backend_x11.py`)
- Uses Xlib RECORD extension for global keyboard capture
- Runs in separate daemon thread
- Tracks modifier states (ctrl, alt, super, shift)
- Emits Qt signals for thread-safe communication
- **Note:** Only works on X11 sessions

**WaylandHotkeyBackend** (`backend_wayland.py`)
- Provides instructions for manual hotkey setup
- Supports GNOME and KDE custom shortcuts
- Returns `False` from `setup()` (manual configuration required)
- **Note:** Full automatic support coming in GNOME 48+

**HotkeyConfig** (`config.py`)
- Defines `HotkeyBinding` dataclass
- Provides preset hotkeys:
  - `super_v`: Super+V (default, Windows-like)
  - `ctrl_alt_v`: Ctrl+Alt+V
  - `super_c`: Super+C
  - `ctrl_shift_v`: Ctrl+Shift+V
- Maps keys to X11 keysyms

#### UI Layer (`copyclip/ui/`)

**MainWindow** (`main_window.py`)
- Main application window (refactored from 999-line ui.py)
- Dependency injection for managers
- Private helper methods with DRY principles
- Methods: `show_clipboard()`, `hide_clipboard()`, `update_clips_display()`
- Keyboard shortcuts: Esc (hide), Ctrl+Esc (quit)

**FirstRunDialog** (`dialogs/first_run.py`)
- Shown on first application launch
- Allows user to select preferred hotkey preset
- Saves selection to settings
- Styled to match application theme

**SettingsDialog** (`dialogs/settings.py`)
- Theme selection (dark/light/system)
- Window pinning toggle
- Hotkey preset changer
- Accessible from main window

**ThemeManager** (`styles/themes.py`)
- Centralized theme definitions with DRY
- Shared stylesheet components (scrollbars, labels)
- Methods: `get_dark_theme()`, `get_light_theme()`, `get_system_theme()`

**ClipFrame** (`widgets/clip_frame.py`)
- Individual clipboard item widget
- Signals: `copy_clicked`, `pin_clicked`
- Text truncation with "show all/show less" toggle
- Displays timestamp and pin indicator

#### Utilities (`copyclip/utils/`)

**constants.py**
- Application-wide constants (eliminates magic numbers)
- `APP_NAME`, `DATA_DIR`, `MAX_CHARS_DISPLAY`, etc.
- Window dimensions and timing constants

**environment.py**
- Detects X11 vs Wayland session
- Functions: `get_session_type()`, `is_x11()`, `is_wayland()`
- Checks `XDG_SESSION_TYPE`, `WAYLAND_DISPLAY`, `DISPLAY`

### Data Flow

1. **Application Start:**
   - `main.py` initializes managers (clipboard, history, settings)
   - First-run check → show `FirstRunDialog` if needed
   - `HotkeyManager` auto-detects X11/Wayland and loads appropriate backend
   - `MainWindow` created with manager dependencies

2. **Clipboard Copy (Ctrl+C detected on X11):**
   - X11 backend detects Ctrl+C → sleeps 0.1s → polls clipboard
   - Emits Qt signal → calls `history_manager.add_item()`
   - History manager checks for duplicates, inserts/moves to top
   - Saves to JSON file

3. **Show Window (Hotkey pressed):**
   - Backend emits `show_clipboard_signal`
   - `main_window.show_clipboard()` called
   - Updates clip display, shows window, focuses search bar

4. **User Interactions:**
   - **Click ClipFrame**: Emits signal → copies to clipboard
   - **Ctrl+Click**: Toggles pin status → redraws display
   - **Search**: Filters clips in real-time

### Threading

- **Main thread**: All PyQt6 UI operations
- **Hotkey thread**: Daemon thread for X11 keyboard monitoring
- **Communication**: PyQt signals for thread-safe events

### File Locations

- **History**: `~/.local/share/clipboard-manager/clipboard_history.json`
- **Settings**: `~/.local/share/clipboard-manager/settings.json`
- **Backup**: `.backup` suffix created before overwriting

## Key Implementation Details

### X11 vs Wayland

**X11:**
- Full global hotkey support via Xlib RECORD extension
- Captures all keyboard events system-wide
- Single Display object used only in record thread

**Wayland:**
- Global hotkeys blocked by design (security)
- Requires manual setup in system settings
- Instructions provided automatically on startup
- Full support coming in GNOME 48+ with global shortcuts portal

### Hotkey Presets

Available presets defined in `HotkeyConfig.PRESETS`:
- Each preset has modifier, key, and display name
- Supports combined modifiers (e.g., "ctrl+alt")
- Mapped to GTK accelerator format for Wayland
- User selects on first run, changeable in settings

### Theme System

- Three themes: dark, light, system
- QSS stylesheets with shared components
- DRY: common styles extracted to class variables
- Applied via `MainWindow.apply_theme()`

### Settings Corruption Handling

- JSON decode errors → backup with timestamp
- Falls back to defaults
- Logs error with helpful message

## Common Development Tasks

### Adding a New Hotkey Preset

1. Add to `HotkeyConfig.PRESETS` in `hotkeys/config.py`
2. Add key to `KEY_KEYSYMS` if needed
3. Update GTK accelerator mapping in `backend_wayland.py` (if applicable)
4. Update `FirstRunDialog` preset descriptions
5. No changes needed in `HotkeyManager` - auto-detected

### Adding a New Theme

1. Create new method in `ThemeManager` (e.g., `get_blue_theme()`)
2. Include all required object names (`#clipFrame`, etc.)
3. Add to dropdown in `SettingsDialog`
4. Update `DEFAULT_SETTINGS` in `constants.py`

### Modifying History Format

1. Update `HistoryItem` TypedDict in `history.py`
2. Update `ClipFrame.__init__()` to handle new fields with fallbacks
3. Consider migration logic in `load_history()` for backward compatibility

## Code Quality Standards

### Type Hints
- All functions have type hints for parameters and return types
- Use `TypedDict` for complex data structures
- Union types (`str | None`) instead of `Optional`

### Linting and Formatting
- Must pass `ruff check` with minimal warnings
- Use `ruff format` before committing
- Line length: 100 characters
- Enabled: E, F, I, UP, B, SIM, PL

### Best Practices
- **DRY**: Extract common patterns to avoid duplication
- **Modularity**: Each component in its own file with clear responsibility
- **Error Handling**: Use `raise ... from err` for exception chaining
- **Docstrings**: All public methods must have docstrings
- **Private Methods**: Prefix with `_` for internal use only

### File Organization
All code in `copyclip/` package with proper `__init__.py` exports.

## Limitations and Future Work

### Current Limitations
- **Wayland**: No automatic global hotkey support (GNOME < 48)
- **Text-only**: No images or rich text support
- **Polling-based**: Uses timer instead of event-driven clipboard monitoring

### Planned Improvements
- Full Wayland support via global shortcuts portal (GNOME 48+)
- Image and rich text clipboard support
- Event-driven clipboard monitoring
- Cross-platform support (Windows, macOS)

## Wayland Setup (Manual)

For users on Wayland, provide these instructions:

**GNOME:**
1. Settings → Keyboard → Custom Shortcuts
2. Add new shortcut:
   - Name: CopyClip
   - Command: `copyclip-show-ui`
   - Shortcut: Ctrl+Alt+V (or preferred)

**KDE:**
1. System Settings → Shortcuts → Custom Shortcuts
2. New → Global Shortcut → Command/URL
3. Set trigger and command: `copyclip-show-ui`

**Note:** Ensure `~/.local/bin` is in PATH or use full path to script.

## Scripts

### Helper Scripts (`scripts/`)

- `install_hotkey.sh`: Registers GNOME custom keybinding (Wayland)
- `register_hotkey.sh`: Core registration logic
- `uninstall_hotkey.sh`: Removes GNOME custom keybinding

**Note:** These scripts are for Wayland users who want automated registration attempts, but manual setup is generally more reliable.
