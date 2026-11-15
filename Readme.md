# A.I.M.S. – Astronaut Integrity & Mission Support (ENTRY TO NASA SPACE APPS CHALLENGE 2026 - Team "SteeBallRun")

**An ESP32-based wearable health-monitoring and mission-support system with a real-time web dashboard.**

A.I.M.S. (Astronaut Integrity & Mission Support) is a prototype designed to demonstrate how wearable technology can assist in monitoring astronaut health, detecting potential injuries, tracking crew activity, and supporting mission operations.

The project combines an ESP32-powered smartband with an onboard REST API and a browser-based dashboard. The smartband collects physiological and motion-related data, while the dashboard visualizes telemetry, crew integrity, injury status, communications, and mission events.

---

## 🚀 Project Overview

A.I.M.S. consists of two primary software components:

1. **ESP32 Smartband (`smartband.ino`)**
   - Collects health and motion data.
   - Detects abnormal vital signs and potential injuries.
   - Provides OLED-based interaction and reminders.
   - Stores historical data locally.
   - Hosts a REST API over Wi-Fi.

2. **Mission Dashboard (`aims.html`)**
   - Connects directly to the ESP32 through its native REST API.
   - Displays live telemetry and crew health information.
   - Provides a visual astronaut injury map.
   - Simulates mission scenarios and emergency conditions.
   - Displays mission logs, communication status, and EVA safety information.

No separate backend server is required for the standard dashboard-to-band Wi-Fi connection.

---

## ✨ Features

### 1. Wearable Health Monitoring

The ESP32 smartband is designed to monitor key physiological parameters.

- Heart rate monitoring (BPM).
- Blood oxygen saturation (SpO2).
- PPG signal acquisition.
- Motion and acceleration monitoring.
- Activity-level estimation.
- Linear acceleration and force calculation.
- Peak force tracking.

The firmware supports both simulated and sensor-based vital readings.

### 2. Injury Detection System

The smartband uses motion data from the MPU6050 accelerometer to estimate acceleration and calculate force.

The system includes:

- Linear acceleration calculation.
- Force estimation using effective mass.
- Peak impact force tracking.
- Configurable injury-force threshold.
- Injury event detection.
- Critical injury alerts.

The dashboard provides a visual representation of the astronaut's body, divided into six regions:

- Head / Helmet
- Torso / Chest
- Left Arm
- Right Arm
- Left Leg
- Right Leg

Injury severity is represented using colour coding:

| Colour | Severity |
|---|---|
| Grey | No detected injury |
| Yellow | Minor |
| Orange | Moderate |
| Red | Severe |

The dashboard also displays the estimated peak force and the source of the injury event.

### 3. Critical Health Alerts

The firmware monitors vital signs for potentially abnormal conditions.

Configured critical thresholds include:

- High heart rate: 120 BPM.
- Low heart rate: 50 BPM.
- Low SpO2: 90%.

Alerts use confirmation intervals and hysteresis to reduce unstable state changes.

The ESP32 also activates its onboard LED when a critical alert is active.

### 4. OLED Interface

The smartband includes a 1.3-inch I2C OLED display using the U8g2 graphics library.

The interface supports:

- Main status screen.
- Interactive event menu.
- Health reminders.
- Mood check-ins.
- Guided health checkups.
- Historical data graphs.
- Vital-sign simulation scenarios.

The BOOT button is used to navigate and interact with the interface.

### 5. Crew Reminders and Mission Support

The firmware includes three recurring reminders:

- Drink water.
- Exercise.
- Routine fluid checkups.

Users can mark reminders as completed or postpone them.

Additional features include:

- Reminder scheduling.
- Snoozing.
- Missed-task logging.
- Guided health checkups.
- Mood tracking using a five-point scale.

The guided health checkup collects measurements over a 15-second interval and records average heart rate, SpO2, activity, and the resulting assessment.

### 6. Historical Data Logging

The ESP32 uses LittleFS to store historical information in flash memory.

The firmware maintains separate CSV files:

| File | Stored information |
|---|---|
| `vitals.csv` | Historical vital-sign measurements |
| `events.csv` | System and mission events |
| `mood.csv` | Crew mood check-ins |
| `tasks.csv` | Reminder completion and missed tasks |
| `checkups.csv` | Health checkup results |

The firmware also maintains graph history in memory and restores stored history during startup.

### 7. Mission Dashboard

The HTML dashboard provides a centralized interface for viewing mission information.

It contains five main sections:

**MISSION LIVE**
- Overall crew integrity score.
- Health score.
- EVA safety score.
- Communications score.
- Latest mission events.
- Emergency simulation controls.
- ESP32 connection controls.

**CREW**
- Astronaut body visualization.
- Injury location selection.
- Injury severity indicators.
- Heart rate.
- SpO2.
- Temperature.
- Force measurements.
- Injury simulation and reset controls.
- Wearable sensor location selection.

**LUNAR MAP**
- Habitat reference point.
- Crew position visualization.
- Distance from habitat.
- EVA safety-radius visualization.
- Colour-coded safety status.

**COMMS**
- Signal strength.
- Packet loss.
- Communication latency.
- Link integrity.
- Animated communication indicators between Moon and Earth.

**MISSION LOG**
- Chronological event history.
- Health events.
- EVA events.
- Communication events.
- Connection events.
- Event category filtering.

### 8. Mission Emergency Simulation

The dashboard includes built-in simulation controls for testing mission scenarios:

- Oxygen emergency.
- EVA safety-radius breach.
- Degrading communications.
- Critical communications failure.
- Restore nominal conditions.

Individual astronaut body regions can also be assigned simulated minor, moderate, or severe injuries.

These features allow the dashboard to demonstrate mission-critical responses without requiring an actual emergency or physical sensor input.

---

## 🔌 System Architecture

The project uses a direct communication architecture.

```text
       ESP32 SMARTBAND
       ┌──────────────────────┐
       │      MPU6050         │
       │  Acceleration Data   │
       │                      │
       │     MAX30102         │
       │  HR / SpO2 / PPG     │
       │  (Optional sensor)   │
       │                      │
       │   OLED Display       │
       │   BOOT Button        │
       │                      │
       │   LittleFS Storage   │
       │                      │
       │   ESP32 REST API     │
       │   /api/telemetry     │
       └──────────┬───────────┘
                  │
             Wi-Fi Network
                  │
                  ▼
       ┌──────────────────────┐
       │    A.I.M.S. WEB      │
       │      DASHBOARD       │
       │                      │
       │  aims.html           │
       │                      │
       │  Live Telemetry      │
       │  Crew Health         │
       │  Injury Visualization│
       │  EVA Monitoring      │
       │  Communications      │
       │  Mission Logs        │
       └──────────────────────┘
```

The ESP32 acts as the data provider and API host. The HTML dashboard retrieves telemetry directly from the device.

---

## 🌐 ESP32 REST API

The ESP32 hosts a lightweight HTTP server on port 80.

### API Endpoint

`GET /api/telemetry`

Example:

```http
GET http://aims-band.local/api/telemetry
```

The API returns a JSON response containing the latest telemetry.

### JSON Response Structure

```json
{
  "crew_id": "crew-1",
  "hr": 72,
  "spo2": 98,
  "activity_g": 0.12,
  "accel_g": 1.02,
  "linear_force_n": 0.45,
  "peak_force_n": 2.10,
  "injury": false,
  "injury_part": "right_arm",
  "simulated": true,
  "uptime_s": 120
}
```

*Example values only.*

### API Parameters

| Parameter | Description |
|---|---|
| `crew_id` | Identifier assigned to the crew member |
| `hr` | Heart rate in BPM |
| `spo2` | Blood oxygen saturation |
| `activity_g` | Activity acceleration |
| `accel_g` | Acceleration magnitude |
| `linear_force_n` | Estimated linear force in Newtons |
| `peak_force_n` | Peak recorded force in Newtons |
| `injury` | Injury alert status |
| `injury_part` | Body region where the band is worn |
| `simulated` | Indicates simulated or sensor-based vital readings |
| `uptime_s` | ESP32 uptime in seconds |

The dashboard polls the API approximately once per second.

The API also includes CORS headers to permit browser-based access.

---

## 🛠️ Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32 DevKit | Main microcontroller |
| MPU6050 | Acceleration and motion sensing |
| MAX30102 (optional) | Heart rate and SpO2 sensing |
| 1.3-inch I2C OLED (SH1106) | Wearable display |
| BOOT button | User interaction |
| USB power source | Power and serial communication |

### Wiring

| Component Pin | ESP32 Pin |
|---|---|
| I2C SDA | GPIO 21 |
| I2C SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |
| Optional external button | GPIO 27 |

The firmware uses the onboard BOOT button (GPIO 0) by default.

---

## 💻 Software Requirements

### ESP32 Firmware

- Arduino IDE.
- ESP32 board support package.
- U8g2 library by olikraus.
- LittleFS (included with the ESP32 core).
- Preferences (included with the ESP32 core).
- Wi-Fi and WebServer libraries (included with the ESP32 core).
- SparkFun MAX3010x library and SpO2 algorithm header when using the physical MAX30102 sensor.

### Web Dashboard

- Modern web browser.
- Google Chrome or Microsoft Edge recommended.
- Wi-Fi connection for API communication.

The dashboard is a standalone HTML file and does not require a web server for local use.

---

## ⚙️ Installation and Setup

### 1. Clone the Repository

```bash
git clone https://github.com/USERNAME/AIMS.git
cd AIMS
```

### 2. Configure the ESP32 Firmware

Open `smartband.ino` in Arduino IDE.

Configure the following settings:

```cpp
#define SIMULATE_VITALS 1
#define AUTO_EVENTS 0

#define ENABLE_AIMS_LINK 1
#define AIMS_PUSH 0
```

- `SIMULATE_VITALS`: Set to `1` for simulated heart rate and SpO2, or `0` for the MAX30102 sensor.
- `AUTO_EVENTS`: Controls whether events are manually triggered or also scheduled automatically.
- `ENABLE_AIMS_LINK`: Enables or disables the Wi-Fi REST API.
- `AIMS_PUSH`: Enables optional POST forwarding to an external backend.

Configure the Wi-Fi credentials and other device settings in the firmware before uploading.

**Important:** Never commit personal Wi-Fi passwords, API keys, or other credentials to a public repository. Use placeholders in the published source.

### 3. Upload the Firmware

1. Connect the ESP32 to your computer.
2. Select the appropriate ESP32 board.
3. Select the correct COM port.
4. Compile and upload the firmware.
5. Open the Serial Monitor at 115200 baud.

Once connected to Wi-Fi, the ESP32 displays its IP address and API endpoint in the Serial Monitor.

The default hostname is:

```text
aims-band.local
```

### 4. Launch the Dashboard

Open `aims.html` directly from your computer.

The dashboard initially operates using simulated data.

To connect through Wi-Fi:

1. Ensure the computer and ESP32 are on the same network.
2. Enter the ESP32 IP address or hostname in the address field.
3. Click **Connect via WiFi**.

The dashboard will begin retrieving telemetry from:

```text
http://aims-band.local/api/telemetry
```

Alternatively, enter the ESP32's local IP address.

### 5. USB Serial Connection (Alternative)

The dashboard also supports direct USB serial communication.

1. Connect the ESP32 using USB.
2. Close the Arduino Serial Monitor.
3. Open the dashboard in Chrome or Edge.
4. Click **Connect ESP32 (USB)**.
5. Select the ESP32 serial port.

The USB interface reads the firmware's serial telemetry output.

---

## 🎮 ESP32 Controls

The onboard BOOT button provides the following interactions:

| Screen | Short Press | Long Press |
|---|---|---|
| Main screen | Open event menu | — |
| Event menu | Next item | Start selected event |
| Reminder | Mark done | Snooze |
| Mood prompt | Change mood | Save mood |
| Health checkup | Start | Postpone |
| Graphs | Next graph | Exit |

A long press is approximately 800 milliseconds.

### Serial Monitor Commands

The firmware also supports commands at 115200 baud.

| Command | Function |
|---|---|
| `y` | Short button press |
| `n` | Long button press |
| `1`–`5` | Record mood |
| `s0`–`s4` | Select vital simulation mode |
| `m` | Trigger mood prompt |
| `c` | Trigger health checkup |
| `g` | Open graphs |
| `l` | List reminders |
| `d` | Dump CSV logs |
| `x` | Erase stored logs |
| `h` | Display help |

---

## 🧪 Testing and Demonstration

A.I.M.S. includes several simulation features to demonstrate the system without requiring actual astronaut conditions.

### Smartband Simulation

The firmware currently defaults to simulated vital readings.

Available scenarios include:

- Normal vital signs.
- High heart rate.
- Low heart rate.
- Low SpO2.

### Dashboard Simulation

The browser dashboard can independently simulate:

- Oxygen emergencies.
- EVA breaches.
- Communications degradation.
- Communications failure.
- Individual body-part injuries.
- Restoration of nominal conditions.

These scenarios are useful for demonstrating the dashboard's alert states, visualizations, and event logging.

---

## 🎯 Potential Applications

Although designed around astronaut monitoring and space mission support, the underlying technologies have potential applications in:

- Astronaut health and physiological monitoring.
- Extravehicular activity (EVA) safety demonstrations.
- Wearable impact and injury monitoring.
- Remote crew-health visualization.
- Emergency-response monitoring systems.
- Industrial worker safety.
- Remote health-monitoring prototypes.
- Human activity and motion tracking.

---

## ⚠️ Current Prototype Limitations

A.I.M.S. is a proof-of-concept prototype and should not be considered a certified medical or aerospace safety system.

Current implementation limitations include:

- Heart rate and SpO2 are simulated by default. Real measurements require enabling and configuring the MAX30102.
- Injury detection is based on experimental acceleration and force thresholds, not clinically validated injury assessment.
- The effective mass and injury thresholds are testing parameters.
- The dashboard's lunar distance, EVA status, and communications metrics are simulated internally rather than received from the ESP32.
- The dashboard's temperature value is simulated.
- The dashboard does not currently retrieve the ESP32's CSV history or historical graphs.
- Wi-Fi API communication uses HTTP rather than encrypted HTTPS.
- The optional backend POST functionality is disabled by default.

The system is intended for experimentation, demonstration, and further development.

---

## 🔮 Future Development Possibilities

Potential improvements include:

- Integration of additional physiological sensors.
- Real-time temperature monitoring.
- GPS or simulated lunar positioning integration.
- Multiple wearable devices and crew identification.
- Centralized mission-control monitoring.
- Historical telemetry visualization.
- Secure API authentication and encrypted communication.
- Improved injury classification algorithms.
- Automated emergency notifications.
- Battery and power-consumption monitoring.

---
Contributers:
Solidsilvr
gladwinyvesmicheal
Waterlaww
AdwaithSachin
