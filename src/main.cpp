#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

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
void setupWiFi();
void reconnect();

const float TEMPERATURE_THRESHOLD = 30.0;
const float OPTIMAL_LUX = 500.0;
const int CO2_THRESHOLD = 600;
const float FAN_SPEED = 30.0;

unsigned long previousMillisTemperature = 0;
const unsigned long temperatureInterval = 1000;

unsigned long previousMillisHumidity = 0;
const unsigned long humidityInterval = 1000;

unsigned long previousMillisLight = 0;
const unsigned long lightInterval = 1000;

unsigned long previousMillisCo2 = 0;
const unsigned long Co2Interval = 1000;

const char *ssid = "ENG-LAB-N2";
const char *password = "12345678";
const char *mqtt_server = "192.168.68.132";
const char *mqtt_user = "michaelng1129";
const char *mqtt_password = "test1234";
const char *mqtt_client_id = "ESP32_Sensor";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastReconnectAttempt = 0;

void setupWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    client.setServer(mqtt_server, 1883);
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void reconnect()
{
  if (millis() - lastReconnectAttempt > 5000)
  {
    lastReconnectAttempt = millis();
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password))
    {
      Serial.println("connected");
      client.setServer(mqtt_server, 1883);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(client.state());
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setupWiFi();
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
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  Serial.println("--------------------------------------------------");
  delay(2000);

  unsigned long currentMillis = millis();
  float temperature = getTemperature();
  float humidity = getHumidity();
  float lightLevel = getLightLevel();
  float tvoc = getTVOC();
  float ch2o = getCH2O();
  int co2 = getCO2();
  uint8_t ledPattern;
  String servoAngle;
  bool atomizationCoolingState = false;
  bool fanState = false;

  if (currentMillis - previousMillisTemperature >= temperatureInterval)
  {
    previousMillisTemperature += temperatureInterval;
    Serial.println("Temperature: " + String(temperature) + " °C");
    if (temperature > TEMPERATURE_THRESHOLD)
    {
      Serial.println("High temperature detected! Activating cooling system.");
      setAtomizationCooling(true);
      atomizationCoolingState = true;
    }
    else
    {
      setAtomizationCooling(false);
      atomizationCoolingState = false;
    }
  }

  if (currentMillis - previousMillisHumidity >= humidityInterval)
  {
    previousMillisHumidity += humidityInterval;
    humidity = constrain(humidity, 0, 100);
    int leftServoAngle = map(humidity, 0, 100, 0, 180);
    int rightServoAngle = map(humidity, 0, 100, 180, 0);
    controlLeftWindowsServo(leftServoAngle);
    controlRightWindowsServo(rightServoAngle);
    servoAngle = String(leftServoAngle) + "," + String(rightServoAngle);

    Serial.println("Humidity: " + String(humidity) + "%");
  }

  if (currentMillis - previousMillisLight >= lightInterval)
  {
    previousMillisLight += lightInterval;

    if (lightLevel >= OPTIMAL_LUX)
    {
      ledPattern = 6;
    }
    else if (lightLevel >= OPTIMAL_LUX * 0.75)
    {
      ledPattern = 4;
    }
    else if (lightLevel >= OPTIMAL_LUX * 0.5)
    {
      ledPattern = 3;
    }
    else if (lightLevel >= OPTIMAL_LUX * 0.25)
    {
      ledPattern = 2;
    }
    else if (lightLevel >= OPTIMAL_LUX * 0.1)
    {
      ledPattern = 1;
    }
    else
    {
      ledPattern = 5;
    }

    Serial.println("Light level: " + String(lightLevel) + " lux, LED pattern: " + String(ledPattern));
    displayPattern(ledPattern);
  }

  if (currentMillis - previousMillisCo2 >= Co2Interval)
  {
    previousMillisCo2 += Co2Interval;
    if (getCO2() > CO2_THRESHOLD)
    {
      Serial.println("High CO2 detected (" + String(co2) + " ppm)! Activating fan at " + String(FAN_SPEED) + "% speed.");
      setFanSpeed(FAN_SPEED);
      fanState = true;
    }
    else
    {
      setFanSpeed(0);
      fanState = false;
    }
    if (updateAirSensorData())
    {
      Serial.println("TVOC: " + String(tvoc) + " mg/m³, " + "CH2O: " + String(ch2o) + " mg/m³, " + "CO2: " + String(co2) + " ppm");
    }
    else
    {
      Serial.println("Failed to read air sensor data.");
    }
  }

  if (client.connected())
  {
    client.publish("esp32/temperature", String(temperature).c_str());
    client.publish("esp32/humidity", String(humidity).c_str());
    client.publish("esp32/atomizationCooling", String(atomizationCoolingState).c_str());
    client.publish("esp32/servoAngle", String(servoAngle).c_str());
    client.publish("esp32/light", String(lightLevel).c_str());
    client.publish("esp32/co2", String(co2).c_str());
    client.publish("esp32/fanStatus", String(fanState).c_str());
    client.publish("esp32/tvoc", String(tvoc).c_str());
    client.publish("esp32/ch2o", String(ch2o).c_str());
  }
}