#include <Arduino.h>
#include <Wire.h>


void setupServo();
void controlLeftWindowsServo(int angle);
void controlRightWindowsServo(int angle);
void setupTaHSensor();
float getTemperature();
float getHumidity();
void setupALSensor();
float getLightLevel();
float getMTreg();
void calibrateLightSensor();
uint16_t XY(uint8_t x, uint8_t y);
void setupLED();
void setLED(uint8_t x, uint8_t y, uint32_t color);
void fillLEDs(uint32_t color);
void updateLEDs();
void clearLEDs();
void displayPattern(uint8_t pattern);
void setupAirSensor();
bool updateAirSensorData();
float getTVOC();
float getCH2O();
float getCO2();



void setup() {
  Serial.begin(115200);
  Wire.begin();
  setupServo();
  setupTaHSensor();
  setupALSensor();
  calibrateLightSensor();
  Serial.println(getMTreg());
  setupLED();
  setupAirSensor();
}

void loop() {

  float temperature = getTemperature();
  float humidity = getHumidity();
  float lightLevel = getLightLevel();

  Serial.println("Temperature: " + String(temperature) + " °C, Humidity: " + String(humidity));
  Serial.println("Light Level: " + String(lightLevel) + " lux");

  controlLeftWindowsServo(90);
  controlRightWindowsServo(90);

  
  delay(1000); 
  displayPattern(6);  // All LEDs

  if (updateAirSensorData()) {
    Serial.print("TVOC: ");
    Serial.print(getTVOC());
    Serial.println(" mg/m³");

    Serial.print("CH2O: ");
    Serial.print(getCH2O());
    Serial.println(" mg/m³");

    Serial.print("CO2: ");
    Serial.print(getCO2());
    Serial.println(" ppm");
  } else {
    Serial.println("Failed to read air sensor data.");
  }

  delay(2000);

}