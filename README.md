# Door alarm system

# How to build the firmware

1. Navigate to the `door_alarm_fw` directory:  
   ```bash
   cd door_alarm_fw
   ```

2. Build the Docker image by executing the following script:  
   ```bash
   ./docker_build.sh
   ```

3. After the Docker image is successfully built, start the container using:  
   ```bash
   ./docker_start.sh
   ```

4. Inside the container, build the firmware by running:  
   ```bash
   ./build_firmware.sh
   ```

5. To program the firmware onto the device, use:  
   ```bash
   ./burn_firmware.sh
   ```

6. If you wish to observe the firmware's output, execute the monitoring script:  
   ```bash
   ./monitor_firmware.sh
   ```


## Release notes

### Version: v2.0
Changelog:
- Component library organised
- Fuse introduced
- IP4283CZ10-TBR,115 ESD diodes

### Version: v1.0
Changelog:
- USB capability (does not work)
- USB active LED (erroneously glows even when operating from battery)
- Debug programming header
- Reed switch gate 
- Debug LED
- Voltage sense
