# odd-macros

A lightweight global shortcut daemon built with **Qt 6**, **KGlobalAccel**, and **libxkbcommon** for Linux (KDE / Wayland & X11). 

`odd-macros` runs silently in the background, listens for user-defined global hotkeys, and triggers virtual keyboard keystrokes via `/dev/uinput` 

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
> `odd-macros` registers shortcut actions with KDE's `KGlobalAccel`. You assign the actual physical hotkeys (e.g., `Meta+Alt+N`) inside **KDE System Settings -> Shortcuts**.

### Example `~/.config/odd-macrosrc`

```ini
# Single key trigger (using key name or raw keycode)
[Shortcuts][mute]
Name=Mute Volume
Keycode=KEY_MUTE

# Simple sequential key combo (presses keys in order)
[Shortcuts][next_sibling]
Name=Goto Next Sibling folder
Keycodes=KEY_BACKSPACE+KEY_RIGHT+KEY_ENTER

# Advanced sequence with key states, delays, and text/mouse actions
[Shortcuts][dolphin_move_files_up]
Name=Dolphin: Cut all files and paste in parent folder
Sequence=down:KEY_LEFTCTRL, click:KEY_A, up:KEY_LEFTCTRL, delay:50, down:KEY_LEFTCTRL, click:KEY_X, up:KEY_LEFTCTRL, delay:100, click:KEY_BACKSPACE, delay:200, down:KEY_LEFTCTRL, click:KEY_V, up:KEY_LEFTCTRL

```

### Configuration Options

| Key | Description | Options / Example |
| --- | --- | --- |
| `Name` | Action name as displayed in KDE System Settings | `Dolphin: Move Files Up` |
| `Keycode` | Single key press. Accepts symbolic key names or raw integer keycodes | `KEY_MUTE` or `113` |
| `Keycodes` | Sequential key presses split by `+` | `KEY_BACKSPACE+KEY_RIGHT+KEY_ENTER` |
| `Sequence` | Advanced multi-step macro syntax. Supports key states, delays, typing, and mouse actions | `down:KEY_LEFTCTRL, click:KEY_A, up:KEY_LEFTCTRL, delay:50` |

### Advanced Sequence Commands

When using `Sequence`, separate actions with commas `,`. Supported commands include:

* `down:KEY_NAME` or `press:KEY_NAME` – Press and hold a key.
* `up:KEY_NAME` or `release:KEY_NAME` – Release a held key.
* `click:KEY_NAME` – Press and release a single key.
* `delay:MS` – Pause execution for the specified milliseconds (e.g., `delay:100`).
* `type:TEXT` – Type out an entire string of text.
* `mouse_move:X;Y` – Move mouse cursor relative to current position.
* `scroll:STEPS` – Scroll mouse wheel (positive for up, negative for down).

> **Tip:** Run `odd-macros -l` to see a full list of supported key names.



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

## Credits & Related Projects

`odd-macros` derived its shortcut registration architecture from concepts originally developed in **[KIOT (KDE - Internet of Things)](https://github.com/davidedmundson/kiot)** by David Edmundson.

### 🔗 Better Together with KIOT
If you use **Home Assistant** or smart home automation, check out **[KIOT](https://github.com/davidedmundson/kiot)**. 

KIOT exposes system events and KDE shortcuts over MQTT to Home Assistant using ultra-fast, event-driven Wayland/Qt mechanisms. Combined with `odd-macros`, you can trigger complex desktop sequence macros directly from your smart home automations!


## License

This project is licensed under the **LGPL-2.1-or-later** License.


