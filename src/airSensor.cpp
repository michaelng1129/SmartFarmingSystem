#include "config.h"
#include <HardwareSerial.h>

HardwareSerial airSensorSerial(2); 

float latestTVOC = 0.0;
float latestCH2O = 0.0;
int latestCO2 = 0;

const int packetSize = 9;

void setupAirSensor()
{
    airSensorSerial.begin(9600, SERIAL_8N1, AIR_RX_PIN, AIR_TX_PIN);
    Serial.println("CO2/VOC/CH2O Sensor started");
}

bool updateAirSensorData() {
    static uint8_t packet[9];
    static uint8_t index = 0;
    static bool headerFound = false;

    // Process all available bytes in the buffer
    while (airSensorSerial.available()) {
        uint8_t byte = airSensorSerial.read();

        if (!headerFound) {
            // Look for the first header byte (0x2C)
            if (byte == 0x2C) {
                packet[0] = byte;
                // Ensure the next byte is available and check if it's 0xE4
                if (airSensorSerial.available()) {
                    byte = airSensorSerial.read();
                    if (byte == 0xE4) {
                        packet[1] = byte;
                        index = 2;
                        headerFound = true;
                    }
                }
            }
            continue;
        }

        // Collect remaining bytes
        packet[index++] = byte;

        // Check if we have a complete packet
        if (index >= packetSize) {
            // Reset state
            index = 0;
            headerFound = false;

            // Verify checksum
            uint8_t checksum = 0;
            for (int i = 0; i < 8; i++) {
                checksum += packet[i];
            }
            checksum &= 0xFF;

            if (checksum != packet[8]) {
                Serial.println("Checksum error");
                // Debug: Print the packet for inspection
                Serial.print("Packet: ");
                for (int i = 0; i < packetSize; i++) {
                    Serial.print(packet[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
                return false;
            }

            // Parse data
            latestTVOC = ((packet[2] << 8) | packet[3]) * 0.001f;
            latestCH2O = ((packet[4] << 8) | packet[5]) * 0.001f;
            latestCO2 = (packet[6] << 8) | packet[7];

            // Debug: Print parsed values
            Serial.print("Parsed - TVOC: ");
            Serial.print(latestTVOC, 3);
            Serial.print(" mg/m³, CH2O: ");
            Serial.print(latestCH2O, 3);
            Serial.print(" mg/m³, CO2: ");
            Serial.print(latestCO2);
            Serial.println(" ppm");

            return true;
        }
    }

    return false;
}

float getTVOC()
{
    return latestTVOC;
}

float getCH2O()
{
    return latestCH2O;
}

int getCO2()
{
    return latestCO2;
}