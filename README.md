<div align="center">

# 📋 CopyClip

</div>

<p align="center">
  <img src="logo.png" alt="logo" width="350"/>
</p>

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GitHub release](https://img.shields.io/github/release/Walkercito/CopyClip.svg)](https://github.com/Walkercito/CopyClip/releases)
[![GitHub issues](https://img.shields.io/github/issues/Walkercito/CopyClip.svg)](https://github.com/Walkercito/CopyClip/issues)
[![Contributions welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg)](https://github.com/Walkercito/CopyClip/issues)

**CopyClip** is an open-source clipboard manager for Linux, inspired by the advanced clipboard features of Windows 10. Easily manage your clipboard history with intuitive controls and quick access to your most recent items.

</div>

## ✨ Features

- 🗂️ **Clipboard History**: Keep track of all copied items in reverse chronological order.
- 📌 **Pin Important Items**: Save crucial snippets by pinning them to the top of your clipboard history.
- 🎛️ **Customizable Hotkeys**: Choose from multiple hotkey presets on first run.
- 🖥️ **X11 Support**: Full global hotkey support on X11 sessions.
- 🔍 **Search**: Quickly filter clipboard items with the built-in search bar.
- 🔄 **Auto-clear on Reboot**: Automatically clear your clipboard history when you restart your computer (with an option to pin items to prevent deletion).
- 🖼️ **Simple UI**: Access your clipboard history through a clean and user-friendly interface.
- 🎨 **Multiple Themes**: Choose between dark, light, or system themes.

## 📦 Dependencies

**CopyClip** requires the following packages to function properly. Install them before running the application:

### Debian/Ubuntu
```bash
sudo apt-get install xsel python3-xlib python3-pyqt6
```

### Fedora
```bash
sudo dnf install xsel python3-xlib python3-pyqt6
```

### Arch/Manjaro
```bash
sudo pacman -S xsel python-xlib python-pyqt6
```

> [!NOTE]
> These dependencies are required for clipboard operations (`xsel`), hotkey detection (`python3-xlib`), and the graphical interface (`python3-pyqt6`). If the app fails to start, verify they're installed correctly.

> [!IMPORTANT]
> **X11 vs Wayland**: Global hotkeys work fully on **X11** sessions. On **Wayland**, you'll need to configure the hotkey manually in your system settings (see Wayland Setup below).

## 🚀 Installation

### Using uv (Recommended)

The project uses [uv](https://docs.astral.sh/uv/) for dependency management:

```bash
# Clone the repository
git clone https://github.com/Walkercito/CopyClip.git
cd CopyClip

# Install uv if you don't have it
curl -LsSf https://astral.sh/uv/install.sh | sh

# Run the application
uv run main.py
```

### Traditional Installation

```bash
# Clone the repository
git clone https://github.com/Walkercito/CopyClip.git
cd CopyClip

# Install Python dependencies
pip3 install PyQt6 python-xlib

# Run the application
python3 main.py
```

## 🛠️ How to Use

### First Run

On first launch, you'll be prompted to select your preferred global hotkey:
- **Super+V** (Windows key + V) - Default, Windows-like
- **Ctrl+Alt+V** - Alternative if Super is in use
- **Super+C** - Alternative with C key
- **Ctrl+Shift+V** - Alternative with Shift modifier

### Basic Usage

1. **Launch CopyClip** using `uv run main.py` or `python3 main.py`
2. **Copy text** as usual using `Ctrl + C`
3. **Open the CopyClip UI** using your configured hotkey (default: `Super + V`)
4. **Navigate your clipboard history**:
   - **Left Click**: Copy item to clipboard
   - **Ctrl + Click**: Toggle pin status
   - **Search**: Use the search bar to filter items
5. **Use the copied content** with `Ctrl + V` in any application

### Keyboard Shortcuts

- `Your Hotkey`: Open/Show CopyClip window (configurable on first run)
- `Esc`: Hide CopyClip window (when visible)
- `Ctrl + Esc`: Terminate application (when window is visible)

## 🖥️ Display Server Support

### X11 (Full Support)
Global hotkeys work automatically on X11 sessions. The application will detect keypresses system-wide.

### Wayland (Manual Setup Required)

On Wayland, you need to configure the global hotkey manually in your system settings:

**GNOME:**
1. Open Settings → Keyboard → View and Customize Shortcuts
2. Scroll to bottom and click "Custom Shortcuts"
3. Click the "+" button
4. Set:
   - **Name**: `CopyClip`
   - **Command**: `copyclip-show-ui`
   - **Shortcut**: Your preferred key combination (e.g., Ctrl+Alt+V)

**KDE Plasma:**
1. Open System Settings → Shortcuts → Custom Shortcuts
2. Edit → New → Global Shortcut → Command/URL
3. Set your preferred trigger and command: `copyclip-show-ui`

> [!NOTE]
> Make sure `~/.local/bin` is in your PATH for the `copyclip-show-ui` command to work. The script `copyclip-show-ui` is automatically created in the project directory.

Alternatively, you can run the UI manually:
```bash
uv run bin/show_ui.py
```

## 🔧 Configuration

CopyClip stores its configuration and history in:
- **Configuration**: `~/.local/share/clipboard-manager/settings.json`
- **History**: `~/.local/share/clipboard-manager/clipboard_history.json`

### Settings

You can customize CopyClip through the Settings dialog (accessible from the main window) or by editing the settings file directly:

```json
{
    "theme": "dark",
    "window_pinned": false,
    "hotkey": "super_v",
    "first_run_completed": true
}
```

**Available settings:**
- **theme**: `dark`, `light`, or `system`
- **window_pinned**: `true` or `false` (keeps window always on top)
- **hotkey**: `super_v`, `ctrl_alt_v`, `super_c`, or `ctrl_shift_v`

## 📁 Project Structure

```
CopyClip/
├── copyclip/               # Main package
│   ├── core/              # Core functionality
│   │   ├── clipboard.py   # Clipboard operations
│   │   ├── history.py     # History management
│   │   └── settings.py    # Settings management
│   ├── hotkeys/           # Hotkey system
│   │   ├── backend_base.py    # Abstract backend
│   │   ├── backend_x11.py     # X11 implementation
│   │   ├── backend_wayland.py # Wayland support
│   │   ├── config.py          # Hotkey configuration
│   │   └── manager.py         # Auto-detection manager
│   ├── ui/                # User interface
│   │   ├── widgets/       # UI components
│   │   ├── dialogs/       # Dialogs (FirstRun, Settings)
│   │   ├── styles/        # Theme management
│   │   └── main_window.py # Main application window
│   └── utils/             # Utilities
│       ├── constants.py   # Application constants
│       └── environment.py # Environment detection
├── bin/                   # Executable scripts
│   ├── show_ui.py         # Manual UI launcher
│   └── copyclip-show-ui   # Standalone launcher for system hotkeys
├── scripts/               # Helper scripts (Wayland setup)
│   ├── ico.py             # Icon generator
│   ├── install_hotkey.sh  # GNOME hotkey installer
│   ├── register_hotkey.sh # Hotkey registration helper
│   └── uninstall_hotkey.sh # Hotkey uninstaller
└── main.py                # Application entry point
```

## 💖 Support the Project

If you find CopyClip useful, consider supporting its development:

<div align="center">
  <a href="https://ko-fi.com/T6T018BZDZ" target="_blank">
    <img src="https://img.shields.io/badge/Support%20me%20on-Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white" alt="Support Me on Ko-fi">
  </a>

  <a href="https://www.paypal.me/KarlaMejiasArian" target="_blank">
    <img src="https://img.shields.io/badge/Donate-PayPal-0079C1?style=for-the-badge&logo=paypal&logoColor=white" alt="Donate with PayPal">
  </a>
</div>

<details>
  <summary><b>🪙 Crypto Donations</b></summary>
  <br>
  <div align="center">
    <img src="https://img.shields.io/badge/Bitcoin-BTC-F7931A?style=for-the-badge&logo=bitcoin&logoColor=white" alt="Bitcoin"><br>
    <code>bc1qhly9zf94ln8wed08d4xrr8q467ef44tx9et963</code>
    <br><br>
    <img src="https://img.shields.io/badge/Ethereum-ETH-3C3C3D?style=for-the-badge&logo=ethereum&logoColor=white" alt="Ethereum"><br>
    <code>0x3b8dde5ae6ac33f0f0884fab40d74488d8426856</code>
    <br><br>
    <img src="https://img.shields.io/badge/Solana-SOL-9945FF?style=for-the-badge&logo=solana&logoColor=white" alt="Solana"><br>
    <code>48CekkeDX6cLABcarL2i4VM9Xz7Xk6ZkAVvbKr5KwLFz</code>
    <br><br>
    <img src="https://img.shields.io/badge/Tron-TRX-FF0013?style=for-the-badge&logo=tron&logoColor=white" alt="Tron"><br>
    <code>TU2ykZsE4rnW5RuXn4Urhg3aunvkCy3Cby</code>
  </div>
</details>

## 🤝 Contributing

We welcome contributions! Here's how you can get involved:

- 🍴 Fork the repository
- 🌿 Create a new branch (`git checkout -b feature-branch`)
- 🛠️ Make your changes
- 💾 Commit your changes (`git commit -m 'Add new feature'`)
- 🚀 Push to the branch (`git push origin feature-branch`)
- 🔁 Open a pull request

### Development

The project uses:
- **uv** for dependency management
- **ruff** for linting and formatting
- **pre-commit** hooks for code quality

```bash
# Install dependencies
uv sync

# Run linting
uv run ruff check .

# Run formatting
uv run ruff format .

# Install pre-commit hooks
uv run pre-commit install
```

See [CLAUDE.md](CLAUDE.md) for detailed development documentation.

## 🗺️ Roadmap

- ✅ **Bug Fix**: Integrate clipboard entries directly with `Ctrl + V` functionality
- ✅ **UI Improvements**: Enhance UI to provide a Windows 11-like experience
- ✅ **Feature**: Configurable hotkeys with multiple presets
- ✅ **Feature**: First-run setup wizard for hotkey selection
- ✅ **Architecture**: Modular, well-organized codebase with proper typing
- ⏳ **Feature**: Full Wayland global hotkey support (requires GNOME 48+)
- ⏳ **Bug Fix**: Text overflow when pressing "show more" button
- 📋 **Feature**: Add support for rich text and image copying
- 🌐 **Feature**: Cross-platform support for Windows and macOS
- 🔒 **Feature**: Optional encryption for sensitive clipboard data

## 📄 License

<div align="center">

  This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

</div>

## 💬 Contact

<div align="center">

  If you encounter any issues or have suggestions, feel free to open an issue or reach out via [GitHub](https://github.com/Walkercito/CopyClip).

</div>

---

<div align="center">

  We hope **CopyClip** helps make your clipboard management easier! ✨

</div>
