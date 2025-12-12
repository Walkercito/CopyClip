<div align="center">

# 📋 CopyClip  
### A modern, open-source clipboard manager for Linux

<img src="logo.png" alt="logo" width="280"/>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Python](https://img.shields.io/badge/Python-3.12-blue?logo=python)
![PyQt6](https://img.shields.io/badge/Qt-PyQt6-41CD52?logo=qt)
[![GitHub release](https://img.shields.io/github/release/Walkercito/CopyClip.svg)](https://github.com/Walkercito/CopyClip/releases)
[![GitHub issues](https://img.shields.io/github/issues/Walkercito/CopyClip.svg)](https://github.com/Walkercito/CopyClip/issues)

</div>

---

## ✨ Overview

**CopyClip** is a lightweight, open-source clipboard manager for Linux, inspired by the Windows 10 clipboard.  
It provides fast access to your clipboard history, pinning, search, customizable hotkeys and a clean desktop-friendly UI.

🖥️ **Currently works on:**  
- **X11 (fully supported)**  
- **Wayland (partial; testers needed)**

> If you're using Wayland, please help us test global hotkey behavior and report issues!

---

## 📸 Screenshots

<div align="center">
  <img src="assets/screenshots/dark-ipsum.png" width="500">
  <br>
  <i>dark mode with text</i>
  <br><br>
  <img src="assets/screenshots/light-ipsum.png" width="500">
  <br>
  <i>light mode with text</i>
</div>


---

## ✨ Key Features

- 🗂️ **History** — Automatic clipboard history in reverse chronological order  
- 📌 **Pinning** — Keep important items always visible  
- 🎛️ **Hotkeys** — Choose your preferred key binding on first launch  
- 🔍 **Instant Search** — Filter items with a built-in search bar  
- 🖥️ **X11 Support** — Global hotkeys work out-of-the-box  
- 🌗 **Themes** — Dark, light, or system  
- 🔄 **Auto-clear on Reboot** — Optional cleanup while preserving pinned items  
- 🖼️ **Clean UI** — Simple, keyboard-friendly interface  

---

## 📦 Dependencies

Install these before running CopyClip:

### Debian / Ubuntu
```bash
sudo apt install xsel wtype xdotool ydotool python3-xlib python3-pyqt6
```

### Fedora
```bash
sudo dnf install xsel wtype xdotool ydotool python3-xlib python3-pyqt6
```

### Arch / Manjaro
```bash
sudo pacman -S xsel wtype xdotool ydotool python-xlib python-pyqt6
```

> **Note:**  
> `xsel` → clipboard operations  
> `xdotool` → auto-paste for X11  
> `ydotool` → auto-paste for Wayland  
> `wtype` → auto-paste alternative for Wayland  
> `python3-xlib` → global hotkeys on X11  
> `PyQt6` → GUI


---

## 🚀 Installation

### Using **uv** (recommended)
```bash
git clone https://github.com/Walkercito/CopyClip.git
cd CopyClip

# Install uv if needed
curl -LsSf https://astral.sh/uv/install.sh | sh

uv run main.py
```

### Using pip
```bash
git clone https://github.com/Walkercito/CopyClip.git
cd CopyClip

pip install PyQt6 python-xlib
python3 main.py
```

---

## 🖥️ Display Server Support

### ✔️ X11 (Full Support)
Global hotkeys work automatically.

### ⚠️ Wayland (Partial – testers needed)
Manual hotkey setup required on GNOME or KDE.

**GNOME**
1. Settings → Keyboard → Custom Shortcuts  
2. Add new:
   - **Name:** CopyClip  
   - **Command:** `copyclip-show-ui`  
   - **Shortcut:** your choice  

**KDE**
1. System Settings → Shortcuts → Custom Shortcuts  
2. Add new → Command/URL → `copyclip-show-ui`

Or launch UI manually:
```bash
uv run bin/show_ui.py
```

> If you're on Wayland, please open an issue with your distro/desktop info to help me improve support.

---

## 🛠️ How to Use

### First Run
Choose your preferred hotkey:
- `Super + V` (default)
- `Ctrl + Alt + V`
- `Super + C`
- `Ctrl + Shift + V`

### Usage
1. Copy text normally (`Ctrl + C`)
2. Open CopyClip with your hotkey  
3. Click to copy items back to clipboard  
4. Ctrl+Click to pin/unpin  
5. Use the search bar to filter items  

### Shortcuts
- **Hotkey** → Open window  
- **Esc** → Hide window  
- **Ctrl + Esc** → Quit
- **Ctrl + Left Click** → Pin / Unpin

---

## 🔧 Config Files

- **Settings:**  
  `~/.local/share/clipboard-manager/settings.json`

- **Clipboard history:**  
  `~/.local/share/clipboard-manager/clipboard_history.json`

Example settings:
```json
{
    # UI preferences
    "theme": "dark",
    "hotkey": "super_v",
    "first_run_completed": False,

    # Behavior settings
    "auto_hide_on_copy": True,
    "auto_paste_on_copy": False,

    # Timing settings (in milliseconds)
    "clipboard_check_interval": 1000,  # Check clipboard every 1 second
    "clipboard_auto_hide_delay": 800,  # Auto-hide delay after copy
    "auto_paste_delay": 200,  # Delay before auto-paste for focus restoration

    # Display settings
    "max_chars_display": 100,  # Maximum characters to display before truncation
    # Environment cache (detected once and cached)

    "window_manager": None,  # Auto-detected: "x11" or "wayland"
    "paste_tool": None,  # Auto-detected: "xdotool", "wtype", or "ydotool"
}
```

---

## 📁 Project Structure

```
CopyClip/
├── copyclip/
│   ├── core/            # Clipboard, history, settings
│   ├── hotkeys/         # X11/Wayland backends + manager
│   ├── ui/              # Widgets, dialogs, styles, main window
│   └── utils/           # Constants and environment helpers
├── bin/                 # CLI utilities and launchers
├── assets/              # Icons and screenshots
├── scripts/             # Hotkey installers and icon tools
└── main.py              # Entry point
```

---

## 🤝 Contributing

Contributions are welcome!

```bash
git checkout -b feature-branch
git commit -m "Add feature"
git push origin feature-branch
```

### Development Tools
- **uv** — dependency management
- **Zed** — as IDE
- **ruff** — linting & formatting  
- **pre-commit** — code quality hooks  

---

## 💖 Support the Project

<div align="center">

<a href="https://ko-fi.com/T6T018BZDZ" target="_blank">
  <img src="https://img.shields.io/badge/Support%20me%20on-Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white">
</a>

<a href="https://www.paypal.me/KarlaMejiasArian" target="_blank">
  <img src="https://img.shields.io/badge/Donate-PayPal-0079C1?style=for-the-badge&logo=paypal&logoColor=white">
</a>

</div>

<details>
  <summary><b>🪙 Crypto Donations</b></summary>
  <br>
  <div align="center">
    <img src="https://img.shields.io/badge/Bitcoin-BTC-F7931A?style=for-the-badge&logo=bitcoin&logoColor=white"><br>
    <code>bc1qhly9zf94ln8wed08d4xrr8q467ef44tx9et963</code>
    <br><br>
    <img src="https://img.shields.io/badge/Ethereum-ETH-3C3C3D?style=for-the-badge&logo=ethereum&logoColor=white"><br>
    <code>0x3b8dde5ae6ac33f0f0884fab40d74488d8426856</code>
    <br><br>
    <img src="https://img.shields.io/badge/Solana-SOL-9945FF?style=for-the-badge&logo=solana&logoColor=white"><br>
    <code>48CekkeDX6cLABcarL2i4VM9Xz7Xk6ZkAVvbKr5KwLFz</code>
    <br><br>
    <img src="https://img.shields.io/badge/Tron-TRX-FF0013?style=for-the-badge&logo=tron&logoColor=white"><br>
    <code>TU2ykZsE4rnW5RuXn4Urhg3aunvkCy3Cby</code>
  </div>
</details>

---

## 📄 License

This project is under the **MIT License**.  
See the [LICENSE](LICENSE) file.

---

<div align="center">

**CopyClip is still evolving. X11 works great. Wayland testers are highly welcome!** 🔥

</div>
