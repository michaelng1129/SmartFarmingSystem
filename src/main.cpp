#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <BH1750.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
#include <driver/mcpwm.h>

unsigned long previousMillisTemperature = 0;
const unsigned long temperatureInterval = 1000;
unsigned long previousMillisHumidity = 0;
const unsigned long humidityInterval = 1000;
unsigned long previousMillisLight = 0;
const unsigned long lightInterval = 1000;
unsigned long previousMillisCo2 = 0;
const unsigned long Co2Interval = 1000;

unsigned long lastReconnectAttempt = 0;

const int NUM_LEDS = 25;
const int BRIGHTNESS = 4;
const int MATRIX_WIDTH = 5;
const int MATRIX_HEIGHT = 5;

const int packetSize = 9;
float latestTVOC = 0.0;
float latestCH2O = 0.0;
int latestCO2 = 0;

WiFiClient espClient;
PubSubClient client(espClient);
Servo leftWindowsServo;
Servo rightWindowsServo;
DHT dht(DHT_PIN, DHT_TYPE);
BH1750 lightMeter;
Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
HardwareSerial airSensorSerial(2);

float TEMPERATURE_THRESHOLD = 30.0;
float OPTIMAL_LUX = 500.0;
int CO2_THRESHOLD = 600;
float FAN_SPEED = 30.0;

// const char *ssid = "ENG-LAB-N2";
// const char *password = "12345678";
const char *ssid = "NCW-Personal";
const char *password = "Ncw5201314";
const char *mqtt_server = "192.168.68.132";
const char *mqtt_user = "michaelng1129";
const char *mqtt_password = "test1234";
const char *mqtt_client_id = "ESP32_Sensor";

void setupServo()
{
  leftWindowsServo.attach(LEFTWINDOWSSERVO_PIN);

  leftWindowsServo.write(0);

  rightWindowsServo.attach(RIGHTWINDOWSSERVO_PIN);

  rightWindowsServo.write(0);
}

void controlLeftWindowsServo(int angle)
{
  leftWindowsServo.write(angle);
}

void controlRightWindowsServo(int angle)
{
  rightWindowsServo.write(angle);
}

void setupTaHSensor()
{
  dht.begin();
}

float getTemperature()
{
  return dht.readTemperature();
}

float getHumidity()
{
  return dht.readHumidity();
}

void setupALSensor()
{
  lightMeter.begin();
  Serial.println(F("BH1750 Test begin"));
}

float getLightLevel()
{
  if (lightMeter.measurementReady(true))
  {
    float lux = lightMeter.readLightLevel();
    if (lux < 0)
    {
      Serial.println(F("Error reading light level"));
      return -1.0;
    }
    return lux;
  }
  else
  {
    Serial.println(F("Measurement not ready"));
    return -1.0;
  }
}

void calibrateLightSensor()
{
  float currentLux = getLightLevel();
  if (currentLux < 0)
  {
    Serial.println(F("Invalid lux for calibration"));
    return;
  }

  if (currentLux > 40000.0)
  {
    if (lightMeter.setMTreg(32))
    {
      Serial.println(F("Calibrate: Set MTReg low for high light"));
    }
    else
    {
      Serial.println(F("Calibrate: Failed to set MTReg low"));
    }
  }
  else if (currentLux > 10.0)
  {
    if (lightMeter.setMTreg(69))
    {
      Serial.println(F("Calibrate: Set MTReg normal for medium light"));
    }
    else
    {
      Serial.println(F("Calibrate: Failed to set MTReg normal"));
    }
  }
  else
  {
    if (lightMeter.setMTreg(138))
    {
      Serial.println(F("Calibrate: Set MTReg high for low light"));
    }
    else
    {
      Serial.println(F("Calibrate: Failed to set MTReg high"));
    }
  }
}

static uint16_t XY(uint8_t x, uint8_t y)
{
  if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT)
    return NUM_LEDS - 1;
  uint16_t i;
  if (y & 0x01)
  {
    // Odd rows run backwards (serpentine)
    uint8_t reverseX = (MATRIX_WIDTH - 1) - x;
    i = (y * MATRIX_WIDTH) + reverseX;
  }
  else
  {
    // Even rows run forwards
    i = (y * MATRIX_WIDTH) + x;
  }
  return i < NUM_LEDS ? i : NUM_LEDS - 1;
}

void setupLED()
{
  // Validate configuration
  if (NUM_LEDS != MATRIX_WIDTH * MATRIX_HEIGHT)
  {
    while (1)
      ; // Halt if configuration is invalid
  }

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
}

void setLED(uint8_t x, uint8_t y, uint32_t color)
{
  if (x < MATRIX_WIDTH && y < MATRIX_HEIGHT)
  {
    strip.setPixelColor(XY(x, y), color);
  }
}

void fillLEDs(uint32_t color)
{
  strip.fill(color, 0, NUM_LEDS);
}

void updateLEDs()
{
  strip.show();
}

void clearLEDs()
{
  strip.clear();
  strip.show();
}

void displayPattern(uint8_t pattern)
{
  strip.clear(); // Clear all LEDs
  for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
  {
    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
    {
      bool shouldLight = false;
      switch (pattern)
      {
      case 0: // Center LED
        shouldLight = (x == 2 && y == 2);
        break;
      case 1: // Center and middle ring
        shouldLight = (x == 2 && y == 2) ||
                      ((x == 1 || x == 3 || y == 1 || y == 3) &&
                       !(x == 0 || x == 4 || y == 0 || y == 4));
        break;
      case 2: // Middle ring (3x3 border)
        shouldLight = ((x == 1 || x == 3 || y == 1 || y == 3) &&
                       !(x == 0 || x == 4 || y == 0 || y == 4));
        break;
      case 3: // Middle ring and outer border
        shouldLight = (x == 0 || x == MATRIX_WIDTH - 1 || y == 0 || y == MATRIX_HEIGHT - 1) ||
                      ((x == 1 || x == 3 || y == 1 || y == 3) &&
                       !(x == 0 || x == 4 || y == 0 || y == 4));
        break;
      case 4: // Outer border
        shouldLight = (x == 0 || x == MATRIX_WIDTH - 1 || y == 0 || y == MATRIX_HEIGHT - 1);
        break;
      case 5: // All LEDs
        shouldLight = true;
        break;
      case 6: // No LEDs
        shouldLight = false;
        break;
      default: // Invalid pattern, do nothing
        return;
      }
      if (shouldLight)
      {
        strip.setPixelColor(XY(x, y), strip.Color(255, 255, 255));
      }
    }
  }
  strip.show();
}

void setupAirSensor()
{
  airSensorSerial.begin(9600, SERIAL_8N1, AIR_RX_PIN, AIR_TX_PIN);
  Serial.println("CO2/VOC/CH2O Sensor started");
}

bool updateAirSensorData()
{
  static uint8_t packet[9];
  static uint8_t index = 0;
  static bool headerFound = false;

  while (airSensorSerial.available())
  {
    uint8_t byte = airSensorSerial.read();

    if (!headerFound)
    {
      if (byte == 0x2C)
      {
        packet[0] = byte;
        if (airSensorSerial.available())
        {
          byte = airSensorSerial.read();
          if (byte == 0xE4)
          {
            packet[1] = byte;
            index = 2;
            headerFound = true;
          }
        }
      }
      continue;
    }

    packet[index++] = byte;
    if (index >= packetSize)
    {
      index = 0;
      headerFound = false;

      uint8_t checksum = 0;
      for (int i = 0; i < 8; i++)
      {
        checksum += packet[i];
      }
      checksum &= 0xFF;

      if (checksum != packet[8])
      {
        Serial.println("Checksum error");
        Serial.print("Packet: ");
        for (int i = 0; i < packetSize; i++)
        {
          Serial.print(packet[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        return false;
      }

      latestTVOC = ((packet[2] << 8) | packet[3]) * 0.001f;
      latestCH2O = ((packet[4] << 8) | packet[5]) * 0.001f;
      latestCO2 = (packet[6] << 8) | packet[7];
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

void setupFanMotor()
{
  pinMode(INA_PIN, OUTPUT);
  pinMode(INB_PIN, OUTPUT);

  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, INA_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, INB_PIN);

  mcpwm_config_t pwm_config_motor;
  pwm_config_motor.frequency = 500;
  pwm_config_motor.cmpr_a = 0;
  pwm_config_motor.cmpr_b = 0;
  pwm_config_motor.counter_mode = MCPWM_UP_COUNTER;
  pwm_config_motor.duty_mode = MCPWM_DUTY_MODE_0;

  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config_motor);
}

void setFanSpeed(float duty_cycle)
{
  if (duty_cycle > 0)
  {
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 0);
  }
  else
  {
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, -duty_cycle);
  }
}

void setupAtomizationCooling()
{
  pinMode(ATOMIZATIONCOOLING_PIN, OUTPUT);
  digitalWrite(ATOMIZATIONCOOLING_PIN, LOW);
}

void setAtomizationCooling(bool state)
{
  if (state)
  {
    digitalWrite(ATOMIZATIONCOOLING_PIN, HIGH);
  }
  else
  {
    digitalWrite(ATOMIZATIONCOOLING_PIN, LOW);
  }
}

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