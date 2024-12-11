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

## Consumption calculations
Power usage estimates assume the application wakes up 6 times a day (3 times for door opening and 3 times for door closing), and the system is powered by 4 AA batteries with average capacity.

| **Component**              | **Current (µA)** | **Time (s)** | **Energy Contribution (µA·s)**   | **Comment**                                                                                          |
|----------------------------|------------------|--------------|-----------------------------------|------------------------------------------------------------------------------------------------------|
| AP2114HA-3.3TRG1           | 60              | 86400        | $$ 60 \times 86400 = 5184000 $$  | LDO operates at all times.                                                                          |
| ESP32 awake doing its job  | 300000          | 60           | $$ 300000 \times 60 = 18000000 $$| Wake-up duration is approximately 10 seconds.                                                      |
| Battery measurement        | 240             | 60           | $$ 240 \times 60 = 14400 $$      | V_SENSE divider: ~ (6 V / 25000 Ω) = 240 µA, active only during wake-up periods.                    |
| Reed switch closed (door open)    | 330             | 45           | $$ 330 \times 45 = 14850 $$      | The door is left open for a maximum of 15 seconds, and this state occurs during half of all activities. |
| ESP32 deep sleep mode      | 8               | 86340        | $$ 8 \times 86340 = 690720 $$    | ESP32 consumes minimal power during deep sleep.                                                     |

#### Total Energy Contribution (µA·s):
$$
5184000 + 18000000 + 14400 + 14850 + 690720 = 23903970 \, \text{µAs}
$$

#### Average Current per day (µA):
$$
\text{Average Current} = \frac{23903970}{86400} \approx 276 \, \text{µA}
$$

**Assumptions:**
- **4 AA batteries** in series: Each AA battery provides **1.5V**, and the combined voltage will be **6V**.
- **Capacity of AA batteries**: We'll assume **2000 mAh** for typical alkaline AA batteries.
- **Average current consumption**: **276 µA** (0.276 mA).


Battery life in hours can be calculated by dividing the total battery capacity by the current draw:
$$
\text{Battery Life (hours)} = \frac{\text{Battery Capacity (mAh)}}{\text{Current Consumption (mA)}}
$$
For **2000 mAh** batteries and **0.276 mA** current:
$$
\text{Battery Life (hours)} = \frac{2000}{0.276} \approx 7246 \, \text{hours}
$$

Convert this to days:
$$
\text{Battery Life (days)} = \frac{7246}{24} \approx 302 \, \text{days}
$$

## Limitations/what could be imporved
- HW routing is not ideal
- Local wifi credentials and static IP must be compiled into the project and must be set from source code
- Consumptions increase when the door is open (around 400-500 uA)
- HW and FW project is tightly-coupled making versioning obscure
- Voltage divider resistor mixed up

