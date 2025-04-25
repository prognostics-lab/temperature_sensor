#include <SPI.h>
#include <Adafruit_MAX31865.h>

// ADC pins D2-D5
#define MAX31865_TOTAL 4          // Total number of sensors
#define MAX31865_CS_PIN_START 2   // First CS pin
#define RTD_NOMINAL 100.0         // PT100
#define RREF 430.0                // Reference resistor

Adafruit_MAX31865 max31865[MAX31865_TOTAL] = {
  Adafruit_MAX31865(MAX31865_CS_PIN_START + 0, 11, 12, 13),
  Adafruit_MAX31865(MAX31865_CS_PIN_START + 1, 11, 12, 13),
  Adafruit_MAX31865(MAX31865_CS_PIN_START + 2, 11, 12, 13),
  Adafruit_MAX31865(MAX31865_CS_PIN_START + 3, 11, 12, 13),
};

uint8_t faults[MAX31865_TOTAL] = {0};

String commandBuffer = ""; // Buffer to store received characters
int max31865_total = sizeof(max31865) / sizeof(Adafruit_MAX31865);

void setup() {
  // Initialize Serial communication at 115200 baud
  Serial.begin(115200);

  // Not sure if this is necessary
  for (int i = 0; i < max31865_total; i++) {
    pinMode(MAX31865_CS_PIN_START + i, OUTPUT);
  }

  Serial.println("=== Initializing ADCs ===");
  for (int i = 0; i < max31865_total; i++) {
    max31865[i].begin(MAX31865_3WIRE);
    Serial.print("Initialized on port D");
    Serial.println(MAX31865_CS_PIN_START + i);
  }

  Serial.println("Rock and stone!");
}

void loop() {
  while (Serial.available() > 0) {
    char received = Serial.read();

    if (received == '\n' || received == '\r') {
      continue;
    }

    commandBuffer += received;
    if (commandBuffer == "R") {
      unsigned long currentTime = millis();

      // Send data
      Serial.print(currentTime);
      for (int i = 0; i < max31865_total; i++) {
        float temperature = max31865[i].temperature(RTD_NOMINAL, RREF);
        Serial.print(",");
        Serial.print(temperature, 2);

        faults[i] = max31865[i].readFault();
      }
      Serial.println("");

      // Check for faults
      for (int i = 0; i < max31865_total; i++) {
          uint8_t fault = faults[i];
          if (fault) {
            Serial.print("Fault @D");
            Serial.print(MAX31865_CS_PIN_START + i);
            Serial.print(" 0x");
            Serial.println(fault, HEX);

            if (fault & MAX31865_FAULT_HIGHTHRESH) {
              Serial.println("RTD High Threshold");
            }
            if (fault & MAX31865_FAULT_LOWTHRESH) {
              Serial.println("RTD Low Threshold");
            }
            if (fault & MAX31865_FAULT_REFINLOW) {
              Serial.println("REFIN- > 0.85 x Bias");
            }
            if (fault & MAX31865_FAULT_REFINHIGH) {
              Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
            }
            if (fault & MAX31865_FAULT_RTDINLOW) {
              Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
            }
            if (fault & MAX31865_FAULT_OVUV) {
              Serial.println("Under/Over voltage");
            }
            max31865[i].clearFault();
          }
      }

      // Clear buffer after processing
      commandBuffer = "";
    } else if (commandBuffer == "*IDN?") {
      Serial.println("ROSATECH,TSN100,P0000003,1.0.1-1");
      commandBuffer = "";
    }
  }

  // Small delay to prevent excessive CPU usage
  delay(10);
}
