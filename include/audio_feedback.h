#pragma once

#include <Arduino.h>
#include "config.h"

enum LedMode {
    LED_MODE_OFF,
    LED_MODE_SCANNING,  // Solid/Blinking Blue
    LED_MODE_SUCCESS,   // Bright Green
    LED_MODE_ERROR,     // Bright Red
    LED_MODE_BUSY       // Yellow / Orange
};

class AudioFeedback {
public:
    static void init();
    static void setLed(bool r, bool g, bool b);
    static void setLedMode(LedMode mode);
    
    static void playTone(uint32_t freq, uint32_t duration_ms);
    static void playTouchClick();
    static void playTagDetected();
    static void playWriteSuccess();
    static void playError();
};
