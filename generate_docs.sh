#!/usr/bin/env bash
set -e

# --- CONFIGURATION ---
DOC_DEPS=(
    "doxygen"
)

OUTPUT_INDEX="docs/html/index.html"

echo "=== Documentation Generator: odd-macros ==="
echo ""

# --- 1. CHECK DISTRO / PACMAN ---
if ! command -v pacman &> /dev/null; then
    echo "ERROR: 'pacman' was not found on this system."
    echo "This script is tailored for Arch Linux / CachyOS."
    exit 1
fi

# --- 2. CHECK & INSTALL DOXYGEN ---
if ! command -v doxygen &> /dev/null; then
    echo "Doxygen is required to generate the HTML documentation, but it is not installed."
    echo "The following package will be installed via pacman:"
    echo "  ${DOC_DEPS[*]}"
    echo ""
    read -p "Do you want to install ${DOC_DEPS[*]} now using sudo pacman? [y/N]: " -n 1 -r
    echo ""

    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo pacman -S --needed "${DOC_DEPS[@]}"
    else
        echo "Aborting documentation generation (Doxygen is missing)."
        exit 1
    fi
else
    echo "✓ Doxygen binary found."
fi

# --- 3. CHECK DOXYFILE ---
if [ ! -f "Doxyfile" ]; then
    echo "ERROR: 'Doxyfile' not found in the current directory."
    echo "Make sure you are running this script from the project root."
    exit 1
fi

# --- 4. RUN DOXYGEN ---
echo ""
read -p "Would you like to build the Doxygen documentation now? [y/N]: " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Building documentation..."
    doxygen Doxyfile

    # --- 5. VERIFY OUTPUT ---
    echo ""
    if [ -f "$OUTPUT_INDEX" ]; then
        echo "✓ Successfully generated documentation!"
        echo "You can open it in your browser using:"
        echo "  xdg-open $OUTPUT_INDEX"
    else
        echo "ERROR: Doxygen finished, but '$OUTPUT_INDEX' was not created."
        exit 1
    fi
else
    echo "Documentation generation cancelled."
    exit 0
fi

echo ""
echo "=== Documentation setup complete! ==="