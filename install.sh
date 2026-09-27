#!/usr/bin/env bash
set -e

echo "=========================================================="
echo " 🚀 ZenithPy One-Line Auto Installer"
echo "=========================================================="

INSTALL_DIR="${HOME}/.zenithpy"
BIN_DIR="${HOME}/.local/bin"
mkdir -p "${BIN_DIR}"

if [ -d "${INSTALL_DIR}" ]; then
    echo "Updating existing installation in ${INSTALL_DIR}..."
    cd "${INSTALL_DIR}"
    git pull origin main
else
    echo "Cloning ZenithPy from GitHub..."
    git clone --depth 1 https://github.com/vashuthegreat7832-lang/zenithpy.git "${INSTALL_DIR}"
    cd "${INSTALL_DIR}"
fi

echo "Compiling raw x86-64 assembly and C native engine..."
make clean > /dev/null 2>&1 || true
make

echo "Setting up global CLI commands ('zenithpy' and 'zenith')..."
ln -sf "${INSTALL_DIR}/bin/zenithpy" "${BIN_DIR}/zenithpy"
ln -sf "${INSTALL_DIR}/bin/zenithpy" "${BIN_DIR}/zenith"

# Export PATH if not already in PATH
if [[ ":$PATH:" != *":${BIN_DIR}:"* ]]; then
    export PATH="${BIN_DIR}:$PATH"
    echo "export PATH=\"${BIN_DIR}:\$PATH\"" >> "${HOME}/.bashrc"
fi

if command -v python3 &>/dev/null; then
    pip install -e . --break-system-packages > /dev/null 2>&1 || python3 setup.py develop --user > /dev/null 2>&1 || true
fi

echo ""
echo "=========================================================="
echo " ✅ Installation Complete!"
echo " You can now run ANY Python script with zero decorators:"
echo ""
echo "    zenithpy filename.py"
echo "    zenith filename.py"
echo "=========================================================="
