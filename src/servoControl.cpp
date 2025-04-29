#include "config.h"
#include <ESP32Servo.h>

Servo leftWindowsServo;
Servo rightWindowsServo;

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