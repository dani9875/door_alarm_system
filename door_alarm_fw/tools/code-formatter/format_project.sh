#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

# Construct the project folder path
PROJECT_FOLDER="$SCRIPT_DIR/../../"
MAIN_PROJECT_FOLDER="$SCRIPT_DIR/../../../"


# Check if the .clang-format file exists in the project folder
if [ ! -f "$SCRIPT_DIR/.clang-format" ]; then
  echo "Error: .clang-format file not found in the project folder."
  exit 1
fi

# List of folders to search for C and header files
SEARCH_FOLDERS=("main" "components")

# Folders to exclude from formatting
EXCLUDE_FOLDERS=("esp_littlefs" "sh_lua")

# Path to the 'cf' executable, one folder deeper in the "tools" directory
CF_EXECUTABLE="$SCRIPT_DIR/cf_v17"

if command -v git &> /dev/null; then
  # Git is available
  # Get list of modified C and header files
  MODIFIED_FILES=$(git -C "$MAIN_PROJECT_FOLDER" status --porcelain | awk '{print $2}' | grep -E '\.(c|h)$')
else
  # Git is not available, format all files within specified folders
  MODIFIED_FILES=$(find "${SEARCH_FOLDERS[@]}" -type f \( -name "*.h" -o -name "*.c" \))
fi

# Loop through each modified file and format it
for FILE in $MODIFIED_FILES; do
  # Check if the file path contains any excluded folders
  EXCLUDE_FILE=false
  for FOLDER in "${EXCLUDE_FOLDERS[@]}"; do
    if [[ "$FILE" == *"$FOLDER"* ]]; then
      EXCLUDE_FILE=true
      break
    fi
  done

  # If the file is not in any excluded folder, and it exists, format it
  if [ "$EXCLUDE_FILE" = false ]; then
    if [ -e "$PROJECT_FOLDER/$FILE" ]; then
      echo "Formatting $FILE..."
      "$CF_EXECUTABLE" -i "$PROJECT_FOLDER/$FILE"  # Use the 'cf' executable with the correct path
    else
      echo "File '$FILE' has been deleted."
    fi
  fi
done
