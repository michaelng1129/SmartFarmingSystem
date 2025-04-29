#include "config.h"
#include <Adafruit_NeoPixel.h>

// LED configuration
#define NUM_LEDS 25
#define BRIGHTNESS 4 // 0-255
#define MATRIX_WIDTH 5
#define MATRIX_HEIGHT 5

// NeoPixel object
static Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

// XY mapping for serpentine matrix layout
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