# odd-macros

A lightweight global shortcut daemon built with **Qt 6**, **KGlobalAccel**, and **libxkbcommon** for Linux (KDE / Wayland & X11). 

`odd-macros` runs silently in the background, listens for user-defined global hotkeys, and triggers virtual keyboard keystrokes via `/dev/uinput` or executes custom system commands.

---

## Features

- **Global Shortcuts:** Seamless integration with KDE's `KGlobalAccel` for system-wide keybindings.
- **Virtual Keyboard Simulation:** Simulates keystrokes and types out full text strings directly through `/dev/uinput`.
- **Background Daemon:** Runs efficiently as a systemd user service (`odd-macros-daemon.service`).
- **Flexible Configuration:** Simple INI configuration format standard to KDE/Qt apps (`~/.config/odd-macrosrc`).

---

## Dependencies

Before building `odd-macros`, ensure you have the following dependencies installed:

### Arch Linux / CachyOS
```bash
sudo pacman -S --needed \
    cmake \
    extra-cmake-modules \
    pkgconf \
    libxkbcommon \
    qt6-base \
    kglobalaccel \
    kconfig \
    base-devel

```

### System Permissions

`odd-macros` requires write access to `/dev/uinput` to inject virtual keyboard events. Your user account must be part of the `input` group:

```bash
sudo usermod -aG input $USER

```

> **Note:** You must log out and log back in for group membership changes to take effect.

---

## Installation

### Method 1: Automated Script (Recommended)

Run the included automated setup script to build, install, set up the user service, and copy the default configuration:

```bash
chmod +x install.sh
./install.sh

```

### Method 2: Manual Build & Install

```bash
# 1. Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 2. Install system-wide (installs binary to /usr/local/bin and service to /usr/local/lib/systemd/user/)
sudo cmake --install build

# 3. Create default config (if not present)
mkdir -p ~/.config
cp examples/odd-macrosrc ~/.config/odd-macrosrc

# 4. Enable and start the user service
systemctl --user daemon-reload
systemctl --user enable --now odd-macros-daemon.service

```

### Method 3: Arch Linux PKGBUILD

If building an Arch Linux package (`.pkg.tar.zst`):

```bash
makepkg -si

```

---

## Configuration

> **How KGlobalAccel works:**  
> `odd-macros` registers shortcut actions (like `next_sibling` or `mute`) with KDE's `KGlobalAccel`. You can assign the actual physical hotkeys (e.g., `Meta+Alt+N`) inside **KDE System Settings -> Shortcuts**.


### Example `~/.config/odd-macrosrc`

```ini
# [Shortcuts] is the top-level group
# [next_sibling] is the unique shortcut ID
[Shortcuts][next_sibling]
Name=Goto Next Sibling folder
Keycodes=14+106+28

# Single keycode macro
[Shortcuts][mute]
Name=Mute Volume
Keycode=113

```

### Configuration Options

| Key | Description | Options / Example |
| --- | --- | --- |
| `shortcut` | Key combination registered with KGlobalAccel | `Meta+Alt+M`, `Ctrl+Shift+F1` |
| `action` | Action performed when shortcut is triggered | `type` or `execute` |
| `text` | Text string to inject via virtual keyboard (when `action=type`) | `Hello World` |
| `command` | Command to run in background (when `action=execute`) | `kitty`, `notify-send "Triggered"` |

---

## Managing the Service

Inspect logs and service status using `systemctl`:

```bash
# Check service status
systemctl --user status odd-macros-daemon.service

# View live logs
journalctl --user -u odd-macros-daemon.service -f

# Restart the service after config updates
systemctl --user restart odd-macros-daemon.service

```

---

## Uninstallation

To remove `odd-macros`, run the included interactive uninstallation script:

```bash
chmod +x uninstall.sh
./uninstall.sh

```

Or remove manually:

```bash
# Stop and disable systemd service
systemctl --user stop odd-macros-daemon.service
systemctl --user disable odd-macros-daemon.service

# Remove installed files
sudo rm -f /usr/local/bin/odd-macros /usr/bin/odd-macros
sudo rm -f /usr/local/lib/systemd/user/odd-macros-daemon.service /usr/lib/systemd/user/odd-macros-daemon.service

# Reload systemd manager configuration
systemctl --user daemon-reload

```

---

## Command Line Interface (CLI)

`odd-macros` functions both as a background daemon and as a command-line tool for key injection and testing.

```text
Usage: odd-macros [options]

Options:
  -h, --help               Displays help on commandline options.
  -v, --version            Displays version information.
  -k, --send-code <code >  Send a single Linux uinput keycode (e.g., 28 for Enter).
  -t, --type <text>        Send a full text string directly through the virtual keyboard.
  -l, --list-keys          Print an overview of all available keycodes and their names.
  -d, --delay <ms>         Delay in milliseconds between keypresses (Default: 12ms).

```

### CLI Usage Examples

```bash
# List all supported keycodes and their corresponding numbers
odd-macros -l

# Send a single keycode (e.g., Keycode 28 = Enter)
odd-macros -k 28

# Type a string via virtual keyboard with custom delay
odd-macros -t "Hello World!" -d 20

# Use in custom shell scripts or automation tools
odd-macros -k 113 # Mute toggle

```

## License

This project is licensed under the **LGPL-2.1-or-later** License.

```
