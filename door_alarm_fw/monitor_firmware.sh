#!/bin/bash

clear 

. $HOME/esp/esp-idf/export.sh

# Get the operating system type
os_type=$(uname)
echo Detected OS: $os_type

# Define the device port based on the operating system type
if [ "$os_type" == "Linux" ]; then
    device_port="/dev/ttyACM*"
elif [ "$os_type" == "Darwin" ]; then
    device_port="/dev/cu.usb*"
else
    echo "Unsupported operating system"
    exit 1
fi

idf.py -p $(ls $device_port) monitor