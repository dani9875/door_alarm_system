# Door alarm system
This project is a smart door alarm system designed to enhance property security by providing real-time monitoring and notifications. The system detects whether a door has been opened or closed and instantly reacts to these events.

When a change is detected, the system transmits the event data through the local network to cloud services. The cloud then relays the information to a mobile application, alerting the property owner via a notification. This ensures the owner is promptly informed whenever someone enters or leaves the property.

# How to build the firmware

1. Navigate to the `door_alarm_fw` directory:  
   ```bash
   cd door_alarm_fw
   ```

2. (Once) Build the Docker image by executing the following script if you have not done yet:  
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

# Technical aspects

## Bring-up

```bash
root@c7cc3ff0b21c:/project# lsusb
Bus 004 Device 001: ID 1d6b:0003  
Bus 003 Device 003: ID 046d:c52b Logitech USB Receiver
Bus 003 Device 002: ID 13d3:54b1 Azurewave Integrated Camera
Bus 003 Device 021: ID 303a:1001 Espressif USB JTAG/serial debug unit
Bus 003 Device 004: ID 0bda:c123 Realtek Bluetooth Radio
Bus 003 Device 001: ID 1d6b:0002  
Bus 002 Device 001: ID 1d6b:0003  
Bus 001 Device 001: ID 1d6b:0002 
```


```bash
void app_main(void)
{
    gpio_reset_pin(led_en);
    gpio_set_direction(led_en, GPIO_MODE_OUTPUT);

    while(1)
    {
        gpio_set_level(led_en, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(led_en, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }   

    return;
}
```

## Limitations/what could be imporved
- HW routing is not ideal
- Local wifi credentials static IP must be compiled into the project
- Consumptions increase when the door is open (around 400-500 uA)
- HW and FW project is tightly-coupled making versioning obscure 

