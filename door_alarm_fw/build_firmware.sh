#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

# echo "Formatting files..."
# # Run the format_project.sh script located in the tools/code-formatter directory
# "$SCRIPT_DIR/tools/code-formatter/format_project.sh"

cd "$SCRIPT_DIR"

# Build the firmware
if idf.py build; then
    # Build succeeded, proceed with flashing OTA partition
    echo -e "\e[32mSuccessful build, you can proceed to burn it onto your target!\e[0m"
else
    # Build failed, display an error message and do not proceed
    echo -e "\e[31mFirmware build failed\e[0m"
    exit 1
fi
