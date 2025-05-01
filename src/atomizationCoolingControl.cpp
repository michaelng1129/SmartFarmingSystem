#include "config.h"
#include <Arduino.h>

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