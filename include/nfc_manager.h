#pragma once

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include "config.h"
#include "openspool_models.h"

enum NfcState {
    NFC_STATE_IDLE,
    NFC_STATE_SEARCHING,
    NFC_STATE_TAG_PRESENT,
    NFC_STATE_ERROR
};

class NfcManager {
public:
    NfcManager();
    bool init();
    
    // Poll for tags non-blockingly (call frequently in main loop)
    bool pollTag(SpoolTag& outTag, bool forceRead = false);

    // Read full OpenSpool tag data from currently detected tag
    bool readTag(SpoolTag& outTag);

    // Write OpenSpool tag data to currently presented tag
    bool writeTag(const SpoolTag& tag, String& outError);

    // Raw NDEF / Hex extraction for diagnostics
    bool readRawNtag(uint8_t* buffer, size_t maxLen, size_t& bytesRead);

    bool isTagPresent() const { return _tagPresent; }
    String getLastUID() const { return _lastUidStr; }
    NfcState getState() const { return _state; }

private:
    HardwareSerial _nfcSerial;
    Adafruit_PN532 _nfc;
    bool _initialized;
    bool _tagPresent;
    NfcState _state;
    uint8_t _lastUid[7];
    uint8_t _lastUidLen;
    String _lastUidStr;
    unsigned long _lastPollTime;

    String formatUID(uint8_t* uid, uint8_t len);
    bool extractJsonFromNdef(const uint8_t* data, size_t dataLen, String& outJson);
    size_t buildNdefPayload(const SpoolTag& tag, uint8_t* outBuffer, size_t maxLen);
};
