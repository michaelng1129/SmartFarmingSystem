#include "config.h"
#include <DHT.h>

DHT dht(DHT_PIN, DHT_TYPE);

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
