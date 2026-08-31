#!/usr/bin/env bash
set -e

# --- CONFIGURATION ---
DEPS=(
    "cmake"
    "extra-cmake-modules"
    "pkgconf"
    "libxkbcommon"
    "qt6-base"
    "kglobalaccel"
    "kconfig"
    "base-devel"
)

SERVICE_NAME="odd-macros-daemon.service"
CONFIG_FILE="$HOME/.config/odd-macrosrc"

echo "=== Setup: odd-macros Daemon ==="
echo ""

# --- 1. CHECK DISTRO / PACMAN ---
if ! command -v pacman &> /dev/null; then
    echo "ERROR: 'pacman' was not found on this system."
    echo "This script is tailored for Arch Linux / CachyOS."
    echo "If you are using a different distribution (Ubuntu, Fedora, etc.), please"
    echo "edit the 'DEPS' array at the top of this script with your package manager's package names."
    exit 1
fi

# --- 2. INSTALL DEPENDENCIES ---
echo "The following packages are required to build the project:"
echo "  ${DEPS[*]}"
echo ""
read -p "Do you want to install the required dependencies? This requires sudo privileges and will run: 'sudo pacman -S --needed ${DEPS[*]}' [y/N]: " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo pacman -S --needed "${DEPS[@]}"
else
    echo "Skipped installing dependencies."
fi

# --- 3. BUILD AND INSTALL THE PROJECT ---
echo ""
read -p "The next step is to build and install the project. Would you like to proceed? [y/N]: " -n 1 -r
echo ""

IS_INSTALLED=false

if [[ $REPLY =~ ^[Yy]$ ]]; then
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j$(nproc)

    echo ""
    read -p "Installing 'odd-macros' system-wide requires sudo privileges ('sudo cmake --install build'). Do you authorize this? [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo cmake --install build
        IS_INSTALLED=true
        echo "✓ Successfully installed binary and systemd service file."
    else
        echo "Skipped installation. The executable is located at 'build/odd-macros'."
    fi
else
    echo "Skipped building."
fi

# --- 4. COPY EXAMPLE CONFIG FILE ---
echo ""
if [ -f "examples/odd-macrosrc" ]; then
    if [ ! -f "$CONFIG_FILE" ]; then
        read -p "No existing configuration found. Would you like to install the example config file to '$CONFIG_FILE'? [y/N]: " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            mkdir -p "$(dirname "$CONFIG_FILE")"
            cp "examples/odd-macrosrc" "$CONFIG_FILE"
            echo "✓ Created config file at '$CONFIG_FILE'."
        fi
    else
        echo "✓ Configuration file already exists at '$CONFIG_FILE'."
    fi
fi

# --- 5. CHECK INPUT GROUP ---
echo ""
if groups "$USER" | grep -q "\binput\b"; then
    echo "✓ User '$USER' is already a member of the 'input' group."
else
    echo "This application requires your user to be in the 'input' group to create virtual keyboard devices (/dev/uinput)."
    read -p "Sudo access is needed to run 'sudo usermod -aG input $USER'. Do you authorize this? [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo usermod -aG input "$USER"
        echo "✓ Added to the 'input' group! NOTE: You must log out and back in for group changes to take effect."
    else
        echo "Warning: The application will fail at runtime without access to uinput/input."
    fi
fi

# --- 6. SYSTEMD USER SERVICE ---
echo ""
if [ "$IS_INSTALLED" = true ]; then
    read -p "Would you like to enable and start the systemd user service now? [y/N]: " -n 1 -r
    echo ""

    if [[ $REPLY =~ ^[Yy]$ ]]; then
        systemctl --user daemon-reload
        systemctl --user enable --now "$SERVICE_NAME"
        echo "✓ Service enabled and started! Check status with: systemctl --user status $SERVICE_NAME"
    fi
else
    echo "Systemd user service setup skipped (project was not installed)."
fi

echo ""
echo "=== Installation and configuration complete! ==="
