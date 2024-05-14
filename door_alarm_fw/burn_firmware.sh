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

./kill_serial_process.sh

esptool.py --chip esp32s3 -p $(ls $device_port) --before=default_reset --after=hard_reset write_flash --flash_mode qio --flash_size detect --flash_freq 40m 0x10000 "build/esp_rfid.bin" 