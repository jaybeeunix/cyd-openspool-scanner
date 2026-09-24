#include "audio_feedback.h"

void AudioFeedback::init() {
    // Initialize RGB LED Pins
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    
    // Set all LEDs to Inactive (High on CYD)
    digitalWrite(LED_RED_PIN, LED_INACTIVE_LEVEL);
    digitalWrite(LED_GREEN_PIN, LED_INACTIVE_LEVEL);
    digitalWrite(LED_BLUE_PIN, LED_INACTIVE_LEVEL);

    // Initialize Buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void AudioFeedback::setLed(bool r, bool g, bool b) {
    digitalWrite(LED_RED_PIN, r ? LED_ACTIVE_LEVEL : LED_INACTIVE_LEVEL);
    digitalWrite(LED_GREEN_PIN, g ? LED_ACTIVE_LEVEL : LED_INACTIVE_LEVEL);
    digitalWrite(LED_BLUE_PIN, b ? LED_ACTIVE_LEVEL : LED_INACTIVE_LEVEL);
}

void AudioFeedback::setLedMode(LedMode mode) {
    switch (mode) {
        case LED_MODE_OFF:
            setLed(false, false, false);
            break;
        case LED_MODE_SCANNING:
            setLed(false, false, true); // Blue
            break;
        case LED_MODE_SUCCESS:
            setLed(false, true, false); // Green
            break;
        case LED_MODE_ERROR:
            setLed(true, false, false); // Red
            break;
        case LED_MODE_BUSY:
            setLed(true, true, false);  // Yellow (Red + Green)
            break;
    }
}

void AudioFeedback::playTone(uint32_t freq, uint32_t duration_ms) {
    if (freq == 0 || duration_ms == 0) return;
    tone(BUZZER_PIN, freq, duration_ms);
    delay(duration_ms);
    noTone(BUZZER_PIN);
}

void AudioFeedback::playTouchClick() {
    tone(BUZZER_PIN, 2400, 15);
}

void AudioFeedback::playTagDetected() {
    setLedMode(LED_MODE_SUCCESS);
    tone(BUZZER_PIN, 1318, 50); // E6
    delay(60);
    tone(BUZZER_PIN, 1568, 80); // G6
    delay(100);
}

void AudioFeedback::playWriteSuccess() {
    setLedMode(LED_MODE_SUCCESS);
    tone(BUZZER_PIN, 1046, 60); // C6
    delay(70);
    tone(BUZZER_PIN, 1318, 60); // E6
    delay(70);
    tone(BUZZER_PIN, 1568, 60); // G6
    delay(70);
    tone(BUZZER_PIN, 2093, 140); // C7
    delay(160);
}

void AudioFeedback::playError() {
    setLedMode(LED_MODE_ERROR);
    tone(BUZZER_PIN, 440, 100);
    delay(120);
    tone(BUZZER_PIN, 330, 180);
    delay(200);
}
