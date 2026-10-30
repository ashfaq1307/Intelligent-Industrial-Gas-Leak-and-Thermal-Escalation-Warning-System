# Intelligent Industrial Gas-Leak and Thermal Escalation Warning System

An ESP32-based IoT project designed to monitor gas levels and temperature, identify potential hazardous conditions, and provide real-time safety status through LED indicators, an LCD, and a web-based dashboard.

The system combines gas sensing, temperature monitoring, hazard classification, and Wi-Fi connectivity to demonstrate an intelligent approach to industrial safety monitoring.

## Overview

Gas leaks and abnormal temperature increases can create hazardous conditions in industrial environments. This project demonstrates a prototype monitoring system that continuously reads sensor values, classifies the detected conditions into different hazard levels, and displays the results locally and through a web dashboard.

The ESP32 processes sensor readings and updates the system status approximately every two seconds.

## Features

- **Gas monitoring:** Reads analog values from an MQ-2 gas sensor.
- **Temperature monitoring:** Measures temperature using an LM35 sensor.
- **Three-level hazard classification:** Classifies conditions as `NORMAL`, `ELEVATED`, or `CRITICAL`.
- **LED-based alerts:** Uses green, yellow, and red LEDs to indicate the current hazard level.
- **LCD display:** Displays sensor readings on a 16×2 I2C LCD.
- **Wi-Fi connectivity:** Connects the ESP32 to a wireless network.
- **Web dashboard:** Displays live sensor readings, hazard status, and connection information.
- **Real-time charts:** Visualizes recent gas and temperature readings in the browser.
- **JSON API:** Exposes sensor readings and system status through an HTTP endpoint.

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 development board | Main controller and Wi-Fi connectivity |
| MQ-2 gas sensor | Gas-related analog sensing |
| LM35 temperature sensor | Temperature measurement |
| Green LED | Indicates normal conditions |
| Yellow LED | Indicates elevated conditions |
| Red LED | Indicates critical conditions |
| 16×2 I2C LCD | Local display of sensor readings |
| Breadboard and jumper wires | Circuit assembly |
| USB power and programming cable | Powering and programming the ESP32 |

**Buzzer status:** The current firmware defines the buzzer as disconnected. The dashboard reports an alarm request during a critical condition, but this does not confirm that a physical audible alarm is operating.

## Software and Technologies

- C++
- Arduino framework
- ESP32
- PlatformIO
- Visual Studio Code
- HTML and CSS
- JavaScript
- HTTP web server
- JSON API

## Pin Configuration

The following GPIO assignments are defined in the firmware.

| Component / Signal | ESP32 GPIO |
|---|---:|
| MQ-2 analog output | GPIO 32 |
| LM35 analog output | GPIO 33 |
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |
| Green LED | GPIO 25 |
| Yellow LED | GPIO 26 |
| Red LED | GPIO 27 |
| Buzzer | Not connected in the current firmware configuration |

**Important:** Verify the sensor output voltages and power requirements before connecting the hardware. ESP32 GPIOs are not 5 V tolerant. Use appropriate voltage conditioning where required, and do not connect a buzzer directly to a GPIO unless its electrical requirements make that safe.

## System Architecture

The system follows this sequence:

1. The MQ-2 sensor provides a gas-related analog reading.
2. The LM35 provides an analog voltage corresponding to temperature.
3. The ESP32 reads and processes both sensor inputs.
4. The firmware classifies the current condition as normal, elevated, or critical.
5. The corresponding LED is activated.
6. The LCD displays the latest gas and temperature readings.
7. When Wi-Fi is connected, the ESP32 serves a web dashboard and a JSON API.
8. The browser periodically requests updated readings and refreshes the dashboard and charts.

## Hazard Classification

The current firmware uses configurable demonstration thresholds.

| Hazard Level | Gas ADC Reading | Temperature |
|---|---:|---:|
| Normal | Below 1000 | Below 35°C |
| Elevated | 1000–1799* | 35–49.9°C* |
| Critical | 1800 or above | 50°C or above |

\*The system evaluates gas and temperature together. If either sensor reaches the critical threshold, the overall status becomes `CRITICAL`. Otherwise, if either reaches the elevated threshold, the status becomes `ELEVATED`. The system reports `NORMAL` when neither threshold is exceeded.

**Note:** These are prototype demonstration thresholds, not certified industrial safety limits. MQ-2 ADC readings are raw analog-to-digital converter values, not calibrated gas concentrations in parts per million (ppm). Appropriate calibration and validation are necessary for meaningful real-world measurements.

## Web Dashboard

Once the ESP32 connects to Wi-Fi, it starts an HTTP server and prints the dashboard address to the serial monitor.

The dashboard includes:

- Current hazard level
- MQ-2 gas sensor ADC reading
- LM35 temperature reading in degrees Celsius
- LED status indicators
- Buzzer command status
- ESP32 connection status
- Gas reading trend chart
- Temperature trend chart
- Last update time

The charts retain up to 60 samples in the browser.

### Accessing the Dashboard

1. Power on the ESP32.
2. Allow it to connect to the configured Wi-Fi network.
3. Open the serial monitor at **115200 baud**.
4. Find the dashboard address printed by the firmware.
5. Open that address in a browser on a device connected to the same accessible network.

The dashboard address depends on the IP assigned to the ESP32 by the network.

## JSON API

The firmware exposes a JSON endpoint at:

`http://<ESP32-IP>/api`

Replace `<ESP32-IP>` with the IP address printed in the serial monitor.

The response contains fields representing:

- `gasADC` — current gas sensor ADC reading
- `temperatureC` — current temperature in degrees Celsius
- `hazard` — current hazard classification
- `readingsAvailable` — whether a sensor sample is available
- `sampleId` — identifier based on the firmware's sampling time counter
- `buzzerCommand` — whether the firmware is requesting an alarm
- `buzzerConnected` — whether the firmware is configured to report a connected buzzer
- `leds` — current green, yellow, and red LED states

Example response:

```json
{
  "gasADC": 850,
  "temperatureC": 29.5,
  "hazard": "NORMAL",
  "readingsAvailable": true,
  "sampleId": 2000,
  "buzzerCommand": false,
  "buzzerConnected": false,
  "leds": {
    "green": true,
    "yellow": false,
    "red": false
  }
}
```

The values above are illustrative examples, not measured results. The actual response depends on the sensor readings and system state.

## Project Setup

### Prerequisites

Install the following software:

- Visual Studio Code
- PlatformIO IDE extension
- ESP32 development platform support through PlatformIO

### 1. Clone the Repository

```bash
git clone <your-repository-url>
cd <your-repository-folder>
```

Replace the placeholders with your actual GitHub repository URL and folder name.

### 2. Configure Wi-Fi

Open `src/main.cpp` and locate the Wi-Fi configuration:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

Replace the placeholders with your local Wi-Fi credentials.

**Security:** Do not commit real Wi-Fi passwords or other secrets to a public GitHub repository. Keep credentials local or load them through a separate, ignored configuration file.

### 3. Check the PlatformIO Configuration

The project's `platformio.ini` should specify the ESP32 board, Arduino framework, serial monitor speed, and required libraries.

For the current project configuration:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    marcoschwartz/LiquidCrystal_I2C
```

### 4. Build and Upload

1. Open the project folder in Visual Studio Code.
2. Wait for PlatformIO to initialize the project.
3. Connect the ESP32 through USB.
4. Build the project using PlatformIO.
5. Upload the firmware to the board.
6. Open the serial monitor at 115200 baud.

### 5. Open the Dashboard

After the ESP32 connects to Wi-Fi, copy the dashboard address printed in the serial monitor into a browser on the same accessible network.

## Project Structure

A typical PlatformIO project structure is:

```text
Intelligent-Industrial-Safety-System/
├── include/
├── lib/
├── src/
│   └── main.cpp
├── test/
├── platformio.ini
└── README.md
```

The exact contents may vary depending on the files included in your repository.

## Limitations

- The project is an educational prototype, not a certified industrial safety device.
- Gas sensor readings are not calibrated ppm measurements.
- Hazard thresholds require calibration and validation for the intended environment.
- Sensor accuracy depends on the hardware, wiring, calibration, and operating conditions.
- The current firmware does not confirm the operation of a physical buzzer.
- Wi-Fi dashboard access depends on successful network connectivity.
- The dashboard's charts retain browser-side sample history rather than providing permanent data storage.
- The prototype should not replace certified industrial gas detection or fire safety equipment.

## Future Improvements

- Add and validate a suitable physical buzzer driver circuit.
- Calibrate gas and temperature readings against appropriate reference equipment.
- Evaluate the hazard thresholds under controlled test conditions.
- Add remote notifications for critical events.
- Introduce persistent logging and historical data analysis.
- Add authentication and secure communication for remote access.
- Evaluate sensor failure handling and network disconnection recovery.

## Safety Disclaimer

This project is intended for educational and demonstration purposes only. It has not been established as a certified industrial safety system. Do not rely on it as the sole means of detecting hazardous gas concentrations, preventing fires, or protecting people and equipment.

## License

No license has been specified yet. If you want others to reuse, modify, and distribute this project, consider choosing an appropriate open-source license and adding a `LICENSE` file to the repository.
