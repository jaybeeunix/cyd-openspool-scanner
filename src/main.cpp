#include <Arduino.h>
#include "config.h"
#include "openspool_models.h"
#include "presets.h"
#include "audio_feedback.h"
#include "nfc_manager.h"
#include "ui_manager.h"

// Global Hardware Managers
static NfcManager nfc;
static UIManager ui;

// Callback when user requests a tag write (from Editor or Cloner)
static void onWriteTagRequested(const SpoolTag& tagToWrite) {
    Serial.printf("[MAIN] Write Requested: %s %s (#%s)\n", 
                  tagToWrite.brand.c_str(), 
                  tagToWrite.type.c_str(), 
                  tagToWrite.color_hex.c_str());
    
    String errorMsg;
    bool ok = nfc.writeTag(tagToWrite, errorMsg);

    if (ok) {
        ui.showStatusMessage("Tag Written OK!", TFT_GREEN, 3000);
        // Refresh dashboard with newly written tag
        SpoolTag verifiedTag = tagToWrite;
        verifiedTag.uid_str = nfc.getLastUID();
        verifiedTag.valid = true;
        ui.setCurrentTag(verifiedTag);
    } else {
        ui.showStatusMessage(errorMsg, TFT_RED, 4000);
        Serial.printf("[MAIN] Write Failed: %s\n", errorMsg.c_str());
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n==========================================");
    Serial.printf("   CYD OpenSpool RFID Writer v%s\n", APP_VERSION);
    Serial.println("==========================================");

    // 1. Initialize Presets Library
    PresetManager::init();

    // 2. Initialize Audio & LED Feedback (RGB LED on CYD, Buzzer on Pin 26)
    AudioFeedback::init();
    AudioFeedback::setLedMode(LED_MODE_SCANNING);

    // 3. Initialize Touchscreen & Display (Portrait 240x320)
    ui.init();
    ui.setWriteRequestedCallback(onWriteTagRequested);

    // 4. Initialize PN532 NFC Module over HSU / UART2
    Serial.println("[MAIN] Initializing PN532 on UART2 (TX=22, RX=35)...");
    if (!nfc.init()) {
        Serial.println("[MAIN] WARNING: PN532 NFC initialization failed!");
        ui.showStatusMessage("PN532 Init Error! Check Wire", TFT_RED, 6000);
        AudioFeedback::setLedMode(LED_MODE_ERROR);
    } else {
        Serial.println("[MAIN] PN532 NFC initialized successfully.");
        ui.showStatusMessage("Ready to Scan", TFT_GREEN, 2500);
        AudioFeedback::setLedMode(LED_MODE_SCANNING);
    }
}

void loop() {
    // 1. Handle Touchscreen Events & GUI Updates
    ui.handleTouch();
    ui.update();

    // 2. Non-blocking Tag Polling
    SpoolTag detectedTag;
    if (nfc.pollTag(detectedTag)) {
        Serial.printf("[MAIN] Tag Read Success: %s %s (#%s) UID: %s\n", 
                      detectedTag.brand.c_str(), 
                      detectedTag.type.c_str(), 
                      detectedTag.color_hex.c_str(),
                      detectedTag.uid_str.c_str());
        
        ui.setCurrentTag(detectedTag);
        ui.showStatusMessage("Tag Read OK!", TFT_GREEN, 2500);
    }

    delay(10);
}
