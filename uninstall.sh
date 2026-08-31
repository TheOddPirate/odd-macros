#!/usr/bin/env bash
set -e

# --- CONFIGURATION ---
SERVICE_NAME="odd-macros-daemon.service"
CONFIG_FILE="$HOME/.config/odd-macrosrc"

# Path arrays for system-wide files (covers standard cmake prefix /usr/local and package prefix /usr)
BINARY_PATHS=(
    "/usr/local/bin/odd-macros"
    "/usr/bin/odd-macros"
)

SERVICE_PATHS=(
    "/usr/local/lib/systemd/user/odd-macros-daemon.service"
    "/usr/lib/systemd/user/odd-macros-daemon.service"
)

echo "=== Uninstall: odd-macros Daemon ==="
echo ""

# --- 1. SYSTEMD USER SERVICE UNINSTALL ---
echo "--- 1. Stopping & Disabling Systemd Service ---"
if systemctl --user is-active --quiet "$SERVICE_NAME" 2>/dev/null || systemctl --user is-enabled --quiet "$SERVICE_NAME" 2>/dev/null; then
    read -p "Would you like to stop and disable the systemd user service ($SERVICE_NAME)? [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        systemctl --user stop "$SERVICE_NAME" 2>/dev/null || true
        systemctl --user disable "$SERVICE_NAME" 2>/dev/null || true
        systemctl --user daemon-reload
        echo "✓ Stopped and disabled systemd user service."
    else
        echo "Skipped stopping systemd service."
    fi
else
    echo "✓ Systemd user service is not active or enabled."
fi

# --- 2. REMOVE SYSTEM-WIDE FILES ---
echo ""
echo "--- 2. Removing Installed Binaries & Service Files ---"

FOUND_FILES=()
for file in "${BINARY_PATHS[@]}" "${SERVICE_PATHS[@]}"; do
    if [ -f "$file" ]; then
        FOUND_FILES+=("$file")
    fi
done

if [ ${#FOUND_FILES[@]} -gt 0 ]; then
    echo "The following system-wide files were detected:"
    for file in "${FOUND_FILES[@]}"; do
        echo "  - $file"
    done
    echo ""

    read -p "Removing 'odd-macros' system-wide requires sudo privileges. Do you authorize this? [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        for file in "${FOUND_FILES[@]}"; do
            sudo rm -f "$file"
        done
        echo "✓ Successfully removed installed binaries and service files."
    else
        echo "Skipped removing system-wide files."
    fi
else
    echo "✓ No installed binary or systemd service files found in /usr/local or /usr."
fi

# --- 3. CONFIGURATION FILE REMOVAL ---
echo ""
echo "--- 3. Configuration File ---"
if [ -f "$CONFIG_FILE" ]; then
    read -p "Would you like to delete your configuration file at '$CONFIG_FILE'? [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f "$CONFIG_FILE"
        echo "✓ Removed configuration file at '$CONFIG_FILE'."
    else
        echo "Kept configuration file at '$CONFIG_FILE'."
    fi
else
    echo "✓ No configuration file found at '$CONFIG_FILE'."
fi

# --- 4. USER GROUP CLEANUP ---
echo ""
echo "--- 4. User Group Membership ---"
if groups "$USER" | grep -q "\binput\b"; then
    echo "User '$USER' is currently a member of the 'input' group."
    read -p "Would you like to remove '$USER' from the 'input' group? (Sudo required) [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo gpasswd -d "$USER" input
        echo "✓ Removed '$USER' from the 'input' group! NOTE: You must log out and back in for group changes to take effect."
    else
        echo "Kept '$USER' in the 'input' group."
    fi
else
    echo "✓ User '$USER' is not in the 'input' group."
fi

echo ""
echo "=== Uninstallation complete! ==="