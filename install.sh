#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # no color

# conf
REPO_URL="https://github.com/Walkercito/CopyClip.git"
INSTALL_DIR="$HOME/.local/share/copyclip"
BIN_DIR="$HOME/.local/bin"
REPO_BRANCH="main"

print_info() {
    echo -e "${BLUE}[  *  ]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[  +  ]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[  !  ]${NC} $1"
}

print_error() {
    echo -e "${RED}[  -  ]${NC} $1"
}

print_header() {
    echo ""
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo -e "${BOLD}${BLUE}  $1${NC}"
    echo -e "${BOLD}${BLUE}========================================${NC}"
    echo ""
}


detect_package_manager() {
    if command -v apt-get &> /dev/null; then
        echo "apt"
    elif command -v dnf &> /dev/null; then
        echo "dnf"
    elif command -v pacman &> /dev/null; then
        echo "pacman"
    else
        echo "unknown"
    fi
}

detect_session_type() {
    if [ -n "$WAYLAND_DISPLAY" ] || [ "$XDG_SESSION_TYPE" = "wayland" ]; then
        echo "wayland"
    elif [ -n "$DISPLAY" ] || [ "$XDG_SESSION_TYPE" = "x11" ]; then
        echo "x11"
    else
        echo "unknown"
    fi
}

install_system_deps() {
    print_header "Installing System Dependencies"

    local pkg_manager=$(detect_package_manager)
    local session_type=$(detect_session_type)

    print_info "Detected package manager: $pkg_manager"
    print_info "Detected session type: $session_type"

    case $pkg_manager in
        apt)
            print_info "Installing dependencies for Debian/Ubuntu..."
            sudo apt-get update
            sudo apt-get install -y git python3 python3-pip xsel python3-xlib curl

            if [ "$session_type" = "wayland" ]; then
                print_info "Installing Wayland tools (wtype recommended)..."
                sudo apt-get install -y wtype || print_warning "wtype not available, will try ydotool"
                sudo apt-get install -y ydotool || print_warning "ydotool not available"
            else
                print_info "Installing X11 tools..."
                sudo apt-get install -y xdotool || print_warning "xdotool not available"
            fi
            ;;
        dnf)
            print_info "Installing dependencies for Fedora..."
            sudo dnf install -y git python3 python3-pip xsel python3-xlib curl

            if [ "$session_type" = "wayland" ]; then
                sudo dnf install -y wtype || print_warning "wtype not available"
                sudo dnf install -y ydotool || print_warning "ydotool not available"
            else
                sudo dnf install -y xdotool || print_warning "xdotool not available"
            fi
            ;;
        pacman)
            print_info "Installing dependencies for Arch/Manjaro..."
            sudo pacman -Sy --noconfirm --needed git python python-pip xsel python-xlib curl

            if [ "$session_type" = "wayland" ]; then
                sudo pacman -S --noconfirm --needed wtype || print_warning "wtype not available"
                sudo pacman -S --noconfirm --needed ydotool || print_warning "ydotool not available"
            else
                sudo pacman -S --noconfirm --needed xdotool || print_warning "xdotool not available"
            fi
            ;;
        *)
            print_error "Unknown package manager. Please install dependencies manually:"
            print_info "  Required: git, python3, xsel, python3-xlib"
            print_info "  Optional (X11): xdotool"
            print_info "  Optional (Wayland): wtype or ydotool"
            exit 1
            ;;
    esac

    print_success "System dependencies installed"
}

install_uv() {
    print_header "Installing uv Package Manager"

    if command -v uv &> /dev/null; then
        print_success "uv is already installed"
        return
    fi

    print_info "Installing uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh

    export PATH="$HOME/.cargo/bin:$PATH"

    if command -v uv &> /dev/null; then
        print_success "uv installed successfully"
    else
        print_error "Failed to install uv"
        exit 1
    fi
}

setup_repository() {
    print_header "Setting Up Repository"

    if [ -d "$INSTALL_DIR" ]; then
        print_info "CopyClip directory exists. Updating..."
        cd "$INSTALL_DIR"
        git fetch origin
        git reset --hard origin/$REPO_BRANCH
        print_success "Repository updated"
    else
        print_info "Cloning CopyClip repository..."
        mkdir -p "$(dirname "$INSTALL_DIR")"
        git clone "$REPO_URL" "$INSTALL_DIR" --branch "$REPO_BRANCH"
        cd "$INSTALL_DIR"
        print_success "Repository cloned"
    fi
}

install_python_deps() {
    print_header "Installing Python Dependencies"

    cd "$INSTALL_DIR"

    print_info "Installing Python packages with uv..."
    uv sync

    print_success "Python dependencies installed"
}

# setup binaries and symlinks
setup_binaries() {
    print_header "Setting Up Executables"

    # create bin directory if it doesn't exist
    mkdir -p "$BIN_DIR"

    # make binaries executable
    chmod +x "$INSTALL_DIR/bin/copyclip-show-ui"
    chmod +x "$INSTALL_DIR/main.py"

    # create symlinks
    print_info "Creating symlinks in $BIN_DIR..."

    ln -sf "$INSTALL_DIR/main.py" "$BIN_DIR/copyclip"
    ln -sf "$INSTALL_DIR/bin/copyclip-show-ui" "$BIN_DIR/copyclip-show-ui"

    print_success "Executables configured"

    # if ~/.local/bin is in PATH
    if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
        print_warning "~/.local/bin is not in your PATH"
        print_info "Add this to your ~/.bashrc or ~/.zshrc:"
        echo "    export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
}

show_instructions() {
    print_header "Installation Complete"

    local session_type=$(detect_session_type)

    print_success "CopyClip has been installed successfully"
    echo ""
    print_info "Installation directory: $INSTALL_DIR"
    print_info "Executables linked in: $BIN_DIR"
    echo ""

    print_header "How to Use"

    if [ "$session_type" = "x11" ]; then
        echo "1. Start CopyClip:"
        echo "   copyclip"
        echo ""
        echo "2. Use the global hotkey:"
        echo "   Default: Super+V (can be changed in settings)"
        echo ""
        echo "3. Or show UI manually:"
        echo "   copyclip-show-ui"

    elif [ "$session_type" = "wayland" ]; then
        echo "1. Start CopyClip:"
        echo "   copyclip"
        echo ""
        print_warning "Wayland requires manual hotkey setup:"
        echo ""

        if command -v gnome-shell &> /dev/null; then
            echo "For GNOME:"
            echo "  1. Open Settings -> Keyboard -> Custom Shortcuts"
            echo "  2. Click + to add a new shortcut"
            echo "  3. Name: CopyClip"
            echo "  4. Command: copyclip-show-ui"
            echo "  5. Shortcut: Press Ctrl+Alt+V (or your preference)"
        elif command -v plasmashell &> /dev/null; then
            echo "For KDE Plasma:"
            echo "  1. Open System Settings -> Shortcuts -> Custom Shortcuts"
            echo "  2. Right-click -> New -> Global Shortcut -> Command/URL"
            echo "  3. Trigger tab: Set your preferred key (e.g., Ctrl+Alt+V)"
            echo "  4. Action tab: Command: copyclip-show-ui"
        else
            echo "  Check your desktop environment's keyboard shortcut settings"
            echo "  Command to run: copyclip-show-ui"
        fi
        echo ""
        echo "3. Or show UI manually:"
        echo "   copyclip-show-ui"
    fi

    echo ""
    print_header "Optional: Run on Startup"
    echo "To start CopyClip automatically on login:"
    echo "  1. Open your desktop environment's Startup Applications"
    echo "  2. Add a new entry:"
    echo "     Name: CopyClip"
    echo "     Command: copyclip"
    echo ""

    print_header "Uninstall"
    echo "To uninstall CopyClip:"
    echo "  rm -rf $INSTALL_DIR"
    echo "  rm $BIN_DIR/copyclip $BIN_DIR/copyclip-show-ui"
    echo "  rm -rf ~/.local/share/clipboard-manager  # Removes settings and history"
    echo ""
}

main() {
    clear
    echo ""
    echo "========================================"
    echo "                                        "
    echo "       CopyClip Installer               "
    echo "       Modern Clipboard Manager         "
    echo "                                        "
    echo "========================================"
    echo ""

    print_info "This script will install CopyClip on your system"
    print_warning "You may need to enter your password for sudo operations"
    echo ""

    read -p "Continue with installation? [Y/n] " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ -n $REPLY ]]; then
        print_error "Installation cancelled"
        exit 1
    fi

    install_system_deps
    install_uv
    setup_repository
    install_python_deps
    setup_binaries
    show_instructions

    echo ""
    print_success "Installation complete. Enjoy using CopyClip!"
    echo ""
}

main
