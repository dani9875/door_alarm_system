#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
RESET='\033[0m'

# Find the process using the serial port
PROCESS=$(ps aux | grep "/bin/bash ./monitor_firmware.sh" | grep -v grep)

if [ -z "$PROCESS" ]; then
    echo -e "${GREEN}No process found using the serial port${RESET}"
else
    PID=$(echo "$PROCESS" | awk '{print $2}')
    echo -e "${BLUE}Process $PID is using the serial port. Killing...${RESET}"
    kill $PID
    echo -e "${GREEN}Process killed.${RESET}"
fi
