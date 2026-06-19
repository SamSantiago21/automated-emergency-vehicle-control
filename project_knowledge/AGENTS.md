# AGENTS.md — mdp-project

## What this is
Arduino sketch for an MDP patent heart rate monitor. Uses up to 4 MAX30102
pulse oximeter sensors via a PCA9548A I2C multiplexer.

## Dependencies (Arduino Library Manager)
- **SparkFun MAX30105** — provides `MAX30105.h`, `heartRate.h`, and `Wire.h`.
  MAX30102 is register-compatible with MAX30105.

## Hardware pinout
- LED: pin 6
- Buzzer: pin 8
- I2C mux address: `0x70`

## Build
Arduino IDE — open `mdp-project.ino` as a sketch. Install SparkFun MAX30105
via Tools → Manage Libraries. Compile and upload normally.

## Serial protocol (115200 baud)
Outputs human-readable status lines and Arduino Serial Plotter data on the
same line: `Plot_IR:<val>,Plot_BPM:<val>`

## Runtime quirks
- 30-second warmup: alarms silenced, `[WARMUP ACTIVE]` printed
- BPM filtered to 30–180 range; alarm thresholds at <40 or >130
- 5-sample rolling average (`rate[5]`)
- I2C channels scanned every 200ms
- 1.5s (`lockTimeout`) lockout on finger removal before scanning other sensors
- Finger detection threshold: IR > 50000; finger removal: IR < 30000
