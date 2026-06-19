# Automated Emergency Vehicle Control — MDP Heart Rate Monitor

## 📋 Description
Arduino-based multi-sensor heart rate monitor using up to **4× MAX30102** pulse oximeter sensors via a **PCA9548A** I2C multiplexer. Detects bradycardia (<40 BPM) and tachycardia (>130 BPM) and triggers visual (LED) and audible (buzzer) alarms. Includes a 30-second warmup period and 5-sample rolling average for stable BPM readings.

---

## 🧰 Parts Required
- 1× Arduino board (Uno, Mega, etc.)
- 1× PCA9548A I2C multiplexer (address `0x70`)
- Up to 4× MAX30102 pulse oximeter sensors
- 1× LED (pin 6)
- 1× Buzzer (pin 8)
- Breadboard & jumper wires

---

## 🔌 Wiring Connections
| Component | Connection |
|-----------|------------|
| PCA9548A SDA → Arduino SDA | A4 (Uno) / pin 20 (Mega) |
| PCA9548A SCL → Arduino SCL | A5 (Uno) / pin 21 (Mega) |
| PCA9548A ADDR pin | GND (address = `0x70`) |
| MAX30102 Ch0–Ch3 | To PCA9548A channels 0–3 |
| LED anode (with resistor) → Pin 6 | LED cathode → GND |
| Buzzer + → Pin 8 | Buzzer − → GND |
| Common power | All VIN → 3.3V / 5V, all GND → common ground |

---

## 📚 Dependencies (Arduino Library Manager)
Install via **Tools → Manage Libraries**:

- **SparkFun MAX30105** — provides `MAX30105.h` and `heartRate.h`.  
  *MAX30102 is register-compatible with MAX30105, so this library works directly.*

No additional libraries are required — `Wire.h` is built into the Arduino core.

---

## ⚙️ Configuration
You can easily change connections or behavior by editing the `#define` constants and variables at the top of `mdp-project.ino`:

| Constant / Variable | Default | Description |
|---------------------|---------|-------------|
| `LED_PIN` | `6` | LED output pin |
| `BUZZER_PIN` | `8` | Buzzer output pin |
| `PCA_ADDR` | `0x70` | I2C address of PCA9548A |
| `TOTAL_SENSORS` | `4` | Number of MAX30102 sensors |
| `lockTimeout` | `1500` | Finger-removal lockout (ms) |
| `warmupDuration` | `30000` | Warmup period (ms) |
| BPM thresholds | `<40` / `>130` | Alarm trigger boundaries |

---

## 🚀 Build & Upload
1. Open `mdp-project.ino` in the Arduino IDE
2. Install **SparkFun MAX30105** via Tools → Manage Libraries
3. Select your board and port under **Tools → Board** and **Tools → Port**
4. Click **Upload** (→) or press `Ctrl+U`

---

## 📡 Serial Protocol (115200 baud)
The sketch outputs human-readable status lines and Arduino Serial Plotter data:

```
Active Ch: 0 | Avg BPM: 72 | Status: NORMAL | Plot_IR:312,Plot_BPM:72.00
```

**Plotter fields:**
- `Plot_IR` — smoothed IR value / 1000
- `Plot_BPM` — average BPM (5-sample rolling)

---

## 📊 Features
- **30-second warmup** — alarms silenced, countdown printed
- **5-sample rolling average** — smooths BPM readings
- **Bradycardia alarm** — triggers at avg BPM < 40
- **Tachycardia alarm** — triggers at avg BPM > 130
- **LED + buzzer alert** — 200ms toggle when abnormal
- **Finger detection** — IR > 50000 = finger present; IR < 30000 = removed
- **1.5s lockout** — prevents rapid sensor switching after finger removal
- **Multi-sensor scan** — auto-scans up to 4 channels every 200ms
