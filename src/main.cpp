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
int getCO2();
void setupFanMotor();
void setFanSpeed(float duty_cycle);
void setupAtomizationCooling();
void setAtomizationCooling(bool state);

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  setupServo();
  setupTaHSensor();
  setupALSensor();
  calibrateLightSensor();
  Serial.println(getMTreg());
  setupLED();
  setupAirSensor();
  setupFanMotor();
  setupAtomizationCooling();
}

void loop()
{
  float temperature = getTemperature();
  float humidity = getHumidity();
  float lightLevel = getLightLevel();

  Serial.println("Temperature: " + String(temperature) + " °C, Humidity: " + String(humidity));
  Serial.println("Light Level: " + String(lightLevel) + " lux");

  controlLeftWindowsServo(90);
  controlRightWindowsServo(90);

  displayPattern(6); // All LEDs

  if (updateAirSensorData())
  {
    Serial.println("TVOC: " + String(getTVOC()) + " mg/m³, " + "CH2O: " + String(getCH2O()) + " mg/m³, " + "CO2: " + String(getCO2()) + " ppm");
  }
  else
  {
    Serial.println("Failed to read air sensor data.");
  }

  delay(2000);
  // setFanSpeed(0);

  setAtomizationCooling(true);
  delay(1000);
  setAtomizationCooling(false);
  delay(10000);
}