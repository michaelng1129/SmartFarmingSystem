#include <BH1750.h>

BH1750 lightMeter;
uint8_t currentMTreg = 69;

void setupALSensor()
{
    //lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
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

float getMTreg()
{
    return currentMTreg;
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
            currentMTreg = 32;
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
            currentMTreg = 69;
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
            currentMTreg = 138;
            Serial.println(F("Calibrate: Set MTReg high for low light"));
        }
        else
        {
            Serial.println(F("Calibrate: Failed to set MTReg high"));
        }
    }
}