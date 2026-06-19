#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>      // Works with MAX30102 (register-compatible, no green LED)
#include "heartRate.h"

// ---------------- Multiplexer ----------------
#define PCA_ADDR 0x70
#define TOTAL_SENSORS 4

// ---------------- Hardware ----------------
#define LED_PIN     6
#define BUZZER_PIN  8

MAX30105 sensor;   

// ---------------- Local Variables ----------------
int activeSensor = -1;  
int lastValidSensor = -1;         
unsigned long fingerRemovalTime = 0; 
const unsigned long lockTimeout = 1500; 

// Warmup Grace Period Configuration
unsigned long systemStartTime = 0;
const unsigned long warmupDuration = 30000; // 30 seconds grace period
unsigned long lastScanTime = 0;             // Prevents I2C bus spamming

float bpm = 0;
float avgBPM = 0;       
float rate[5] = {0};

int rateIndex = 0;
long lastBeat = 0;
long smoothIR = 0;

// =====================================================
// Multiplexer
// =====================================================
void selectChannel(uint8_t channel) {
    Wire.beginTransmission(PCA_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

// =====================================================
// Sensor Initialization (MAX30102)
// =====================================================
void initSensor(uint8_t ch) {
    selectChannel(ch);
    if (!sensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println(F("[ERROR] Sensor Initialization Failed!"));
        while (1);
    }
    sensor.setup();                      
    sensor.setPulseAmplitudeIR(0x1F);
    sensor.setPulseAmplitudeRed(0x0A);
}

// =====================================================
// Alert Manager
// =====================================================
void updateAlerts(bool triggerAlarm) {
    static unsigned long lastToggle = 0;
    static bool state = false;

    if (triggerAlarm) {
        if (millis() - lastToggle >= 200) {
            lastToggle = millis();
            state = !state;
            digitalWrite(LED_PIN, state);
            digitalWrite(BUZZER_PIN, state);
        }
    } else {
        state = false;
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
    }
}

// =====================================================
// Setup
// =====================================================
void setup() {
    Wire.begin();
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    Serial.println(F("Initializing System..."));

    activeSensor = -1;
    avgBPM = 0.0;

    delay(2000);
    
    systemStartTime = millis(); 
}

// =====================================================
// Main Loop
// =====================================================
void loop() {
    long ir;
    bool isWarmingUp = (millis() - systemStartTime < warmupDuration);

    // -------------------------------------
    // Search for Active Sensor
    // -------------------------------------
    if (activeSensor == -1) {
        // Only scan channels every 200ms to avoid locking up the I2C bus
        if (millis() - lastScanTime >= 200) {
            lastScanTime = millis();

            Serial.print(F("Status: Place Finger | Searching... "));
            if (isWarmingUp) {
                Serial.print(F("[WARMUP ACTIVE: Alarms Silenced - "));
                Serial.print((warmupDuration - (millis() - systemStartTime)) / 1000);
                Serial.println(F("s left]"));
                updateAlerts(false); 
            } else {
                Serial.println(F(""));
                updateAlerts(true);
            }

            for (int i = 0; i < TOTAL_SENSORS; i++) {
                if (lastValidSensor != -1 && i != lastValidSensor) {
                    if (millis() - fingerRemovalTime < lockTimeout) {
                        continue; 
                    }
                }

                selectChannel(i);
                if (sensor.begin(Wire, I2C_SPEED_STANDARD)) {
                    sensor.setup();
                    ir = sensor.getIR();

                    if (ir > 50000) {
                        activeSensor = i; 
                        lastValidSensor = i; 
                        initSensor(i);
                        Serial.print(F("Status: Locked onto Sensor Channel: "));
                        Serial.println(i);
                        delay(1000);
                        break;
                    }
                }
            }
        }
        return;
    }

    // -------------------------------------
    // Read Active Sensor
    // -------------------------------------
    selectChannel(activeSensor);
    ir = sensor.getIR();

    if (ir < 30000) {
        Serial.print(F("Status: Finger removed from Ch "));
        Serial.print(activeSensor);
        Serial.println(F(". Resetting data values..."));
        
        avgBPM = 0;        
        bpm = 0;
        rateIndex = 0;
        for (int i = 0; i < 5; i++) {
            rate[i] = 0; 
        }
        
        activeSensor = -1; 
        fingerRemovalTime = millis(); 
        
        updateAlerts(false); 
        delay(300);
        return; 
    }

    // -------------------------------------
    // BPM Calculation
    // -------------------------------------
    if (checkForBeat(ir)) {
        long delta = millis() - lastBeat;
        lastBeat = millis();
        bpm = 60.0 / (delta / 1000.0);

        if (bpm > 30 && bpm < 180) {
            rate[rateIndex++] = bpm;
            rateIndex %= 5;

            float tempAvg = 0;
            for (int i = 0; i < 5; i++) {
                tempAvg += rate[i];
            }
            avgBPM = tempAvg / 5; 
        }
    }

    // -------------------------------------
    // Serial Status Text Reporting
    // -------------------------------------
    Serial.print(F("Active Ch: "));
    Serial.print(activeSensor);
    Serial.print(F(" | Avg BPM: "));
    Serial.print((int)avgBPM);
    Serial.print(F(" | Status: "));

    if (avgBPM > 0) {
        if (avgBPM < 40) {
            Serial.print(F("LOW HEART RATE"));
        } else if (avgBPM > 130) {
            Serial.print(F("HIGH HEART RATE"));
        } else {
            Serial.print(F("NORMAL"));
        }
    } else {
        Serial.print(F("Calculating (BPM is 0.0)..."));
    }
    
    if (isWarmingUp) {
        Serial.print(F(" [WARMUP ACTIVE]"));
    }
    Serial.print(F(" | "));

    // -------------------------------------
    // Evaluated Alert Logic
    // -------------------------------------
    bool abnormal = (avgBPM > 0 && (avgBPM < 40 || avgBPM > 130)) || (avgBPM == 0 && activeSensor != -1);
    
    if (isWarmingUp) {
        updateAlerts(false);
    } else {
        updateAlerts(abnormal);
    }

    // -------------------------------------
    // Serial Plotter Output Data
    // -------------------------------------
    smoothIR = (smoothIR * 0.8) + (ir * 0.2);
    int plotIR = smoothIR / 1000;

    Serial.print(F("Plot_IR:"));
    Serial.print(plotIR);
    Serial.print(F(",Plot_BPM:"));
    Serial.println(avgBPM);

    delay(10);
}
