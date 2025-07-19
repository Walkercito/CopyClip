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
- 🎛️ **Customizable Shortcuts**: Easily configure shortcuts for quick access and management.
- 🔄 **Auto-clear on Reboot**: Automatically clear your clipboard history when you restart your computer (with an option to pin items to prevent deletion).
- 🖼️ **Simple UI**: Access your clipboard history through a clean and user-friendly interface.
- 🎨 **Multiple Themes**: Choose between dark, light, or system themes.

## 📦 Dependencies

**CopyClip** requires the following packages to function properly. Install them before running the application:

### Debian/Ubuntu
```bash
sudo apt-get install xsel python3-xlib python3-pyqt6 python3-pip
```

### Fedora
```bash
sudo dnf install xsel python3-xlib python3-pyqt6 python3-pip
```

### Arch/Manjaro
```bash
sudo pacman -S xsel python-xlib python-pyqt6 python-pip
```

> [!NOTE]  
> These dependencies are required for clipboard operations (`xsel`), hotkey detection (`python3-xlib`), and the graphical interface (`python3-pyqt6`). If the app fails to start, verify they're installed correctly.

## 🚀 Installation

### Option 1: Download Pre-compiled Binary (Recommended)

1. Go to the [Releases](https://github.com/Walkercito/CopyClip/releases) page
2. Download the latest `CopyClip` binary for your system
3. Make it executable:
   ```bash
   chmod +x CopyClip
   ```
4. Run the application:
   ```bash
   ./CopyClip
   ```

### Option 2: Build from Source

#### Step 1: Clone the repository
```bash
git clone https://github.com/Walkercito/CopyClip.git
cd CopyClip
```

#### Step 2: Install Python dependencies
```bash
pip3 install PyQt6 Pillow Xlib
```

#### Step 3: Run from source
```bash
python3 main.py
```

#### Step 4: (Optional) Create your own executable
If you want to create your own standalone executable:

```bash
# Install PyInstaller
pip3 install pyinstaller

# Create the executable
pyinstaller --onefile --windowed --name=CopyClip main.py

# The executable will be in the dist/ folder
cd dist/
./CopyClip
```

## 🛠️ How to Use

1. **Launch CopyClip** using one of the installation methods above.
2. **Copy text** as usual using `Ctrl + C`.
3. **Open the CopyClip UI** using the default shortcut `Super + V` (Windows key + V).
4. **Navigate your clipboard history**:
   - **Left Click**: Copy item to clipboard
   - **Ctrl + Left Click**: Toggle pin status
   - **Search**: Use the search bar to filter items
5. **Use the copied content** with `Ctrl + V` in any application.

### Keyboard Shortcuts

- `Super + V`: Open/Show CopyClip window
- `Esc`: Hide CopyClip window (when visible)
- `Ctrl + Esc`: Terminate application (when window is visible)

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

## 🔧 Configuration

CopyClip stores its configuration and history in:
- **Configuration**: `~/.local/share/clipboard-manager/settings.json`
- **History**: `~/.local/share/clipboard-manager/clipboard_history.json`

### Settings

You can customize CopyClip through the Settings dialog (accessible from the main window) or by editing the settings file directly:

```json
{
    "theme": "dark",
    "window_pinned": false
}
```

Available themes: `dark`, `light`, `system`

## 🤝 Contributing

We welcome contributions! Here's how you can get involved:

- 🍴 Fork the repository.
- 🌿 Create a new branch (`git checkout -b feature-branch`).
- 🛠️ Make your changes.
- 💾 Commit your changes (`git commit -m 'Add new feature'`).
- 🚀 Push to the branch (`git push origin feature-branch`).
- 🔁 Open a pull request.

## 🗺️ Roadmap

- 🛠️ **Bug Fix**: Integrate clipboard entries directly with `Ctrl + V` functionality. <span style="color:green"><b>[FIXED]</b></span>
- 💻 **UI Improvements**: Enhance UI to provide a Windows 11-like experience. <span style="color:green"><b>[FIXED]</b></span>
- ⚠️ **Bug Fix**: Text overflows screen when pressing "show more" button. <span style="color:orange"><b>[IN PROGRESS]</b></span>
- ✒️ **Feature**: Add support for rich text and image copying.
- 🌐 **Feature**: Cross-platform support for Windows and macOS.
- 🔧 **Feature**: Configurable hotkeys.
- 🔒 **Feature**: Optional encryption for sensitive clipboard data.

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
