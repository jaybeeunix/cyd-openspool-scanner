#include "nfc_manager.h"
#include "audio_feedback.h"

NfcManager::NfcManager() 
    : _nfcSerial(PN532_UART_NUM), 
      _nfc(PN532_RESET_PIN, &_nfcSerial),
      _initialized(false),
      _tagPresent(false),
      _state(NFC_STATE_IDLE),
      _lastUidLen(0),
      _lastPollTime(0) {
}

bool NfcManager::init() {
    _nfcSerial.begin(PN532_BAUDRATE, SERIAL_8N1, PN532_RX_PIN, PN532_TX_PIN);
    _nfc.begin();

    uint32_t versiondata = _nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[NFC] Failed to find PN532 board via HSU UART2");
        _state = NFC_STATE_ERROR;
        _initialized = false;
        return false;
    }

    Serial.printf("[NFC] Found PN532 chip: PN5%02X, Firmware ver %d.%d\n", 
                  (versiondata >> 24) & 0xFF, 
                  (versiondata >> 16) & 0xFF, 
                  (versiondata >> 8) & 0xFF);

    // Configure SAM (Secure Access Module) in normal mode
    _nfc.SAMConfig();
    _nfc.setPassiveActivationRetries(0x10); // Don't block indefinitely

    _initialized = true;
    _state = NFC_STATE_SEARCHING;
    return true;
}

String NfcManager::formatUID(uint8_t* uid, uint8_t len) {
    String formatted = "";
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) formatted += ":";
        if (uid[i] < 0x10) formatted += "0";
        formatted += String(uid[i], HEX);
    }
    formatted.toUpperCase();
    return formatted;
}

bool NfcManager::pollTag(SpoolTag& outTag, bool forceRead) {
    if (!_initialized) return false;

    // Throttle polling frequency (every 150ms)
    unsigned long now = millis();
    if (now - _lastPollTime < 150 && !forceRead) {
        return false;
    }
    _lastPollTime = now;

    // Clear any residual UART bytes before starting card detection
    while (_nfcSerial.available()) {
        _nfcSerial.read();
    }

    uint8_t uid[7] = {0};
    uint8_t uidLen = 0;

    // Quick non-blocking read attempt
    bool success = _nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 60);

    if (success && uidLen > 0) {
        String newUidStr = formatUID(uid, uidLen);
        bool isNewTag = (newUidStr != _lastUidStr) || !_tagPresent || forceRead;

        memcpy(_lastUid, uid, uidLen);
        _lastUidLen = uidLen;
        _lastUidStr = newUidStr;
        _tagPresent = true;
        _state = NFC_STATE_TAG_PRESENT;

        if (isNewTag) {
            Serial.printf("[NFC] New Tag Detected: %s (len: %d)\n", _lastUidStr.c_str(), uidLen);
            if (readTag(outTag)) {
                outTag.uid_str = _lastUidStr;
                AudioFeedback::playTagDetected();
                return true;
            }
        }
    } else {
        if (_tagPresent) {
            Serial.println("[NFC] Tag Removed");
            _tagPresent = false;
            _lastUidStr = "";
            _state = NFC_STATE_SEARCHING;
            AudioFeedback::setLedMode(LED_MODE_SCANNING);
        }
    }

    return false;
}

bool NfcManager::readTag(SpoolTag& outTag) {
    if (!_initialized) return false;

    // Flush residual bytes from readPassiveTargetID (2 extra bytes left on 7-byte UID)
    while (_nfcSerial.available()) {
        _nfcSerial.read();
    }
    delay(5);

    // Buffer to hold user memory pages from NTAG215 (up to 40 pages = 160 bytes or 60 pages = 240 bytes)
    const uint8_t MAX_PAGES_TO_READ = 60;
    uint8_t pageBuffer[4];
    uint8_t memoryDump[MAX_PAGES_TO_READ * 4];
    size_t totalBytes = 0;

    // NTAG215 user memory starts at Page 4
    for (uint8_t page = 4; page < (4 + MAX_PAGES_TO_READ); page++) {
        while (_nfcSerial.available()) {
            _nfcSerial.read();
        }

        if (_nfc.ntag2xx_ReadPage(page, pageBuffer)) {
            memcpy(&memoryDump[totalBytes], pageBuffer, 4);
            totalBytes += 4;
            
            // Check for NDEF terminator TLV (0xFE)
            bool foundTerminator = false;
            for (int b = 0; b < 4; b++) {
                if (pageBuffer[b] == 0xFE) {
                    foundTerminator = true;
                    break;
                }
            }
            if (foundTerminator) break;
        } else {
            // Read error or reached unwritten page boundary
            break;
        }
    }

    if (totalBytes == 0) {
        Serial.println("[NFC] Failed to read user memory pages - tag may be unformatted or moved");
        // Still register tag on dashboard so user can write to it
        outTag.reset();
        outTag.uid_str = _lastUidStr;
        outTag.brand = "Blank Tag";
        outTag.type = "Unformatted";
        outTag.color_hex = "808080";
        outTag.valid = true;
        return true;
    }

    Serial.printf("[NFC] Successfully read %d bytes of user memory\n", totalBytes);

    String jsonPayload;
    if (extractJsonFromNdef(memoryDump, totalBytes, jsonPayload)) {
        Serial.printf("[NFC] Extracted JSON: %s\n", jsonPayload.c_str());
        if (outTag.fromJSON(jsonPayload)) {
            outTag.uid_str = _lastUidStr;
            return true;
        }
    } else {
        Serial.println("[NFC] Tag contains data but no OpenSpool JSON (or Blank). Ready for writing.");
        outTag.reset();
        outTag.uid_str = _lastUidStr;
        outTag.brand = "Blank Tag";
        outTag.type = "Ready to Write";
        outTag.color_hex = "00AE42";
        outTag.valid = true;
        return true;
    }

    return false;
}

bool NfcManager::extractJsonFromNdef(const uint8_t* data, size_t dataLen, String& outJson) {
    if (!data || dataLen < 5) return false;

    // Method 1: Standard NDEF TLV Parser
    // Find NDEF TLV (0x03)
    size_t idx = 0;
    while (idx < dataLen) {
        if (data[idx] == 0x03) { // NDEF Message TLV Tag
            idx++;
            if (idx >= dataLen) break;
            
            size_t ndefLen = data[idx++];
            if (ndefLen == 0xFF) { // 3-byte length format
                if (idx + 1 >= dataLen) break;
                ndefLen = (data[idx] << 8) | data[idx + 1];
                idx += 2;
            }

            if (idx >= dataLen) break;

            // NDEF Record Header (e.g. 0xD2 for MIME Short Record)
            uint8_t header = data[idx++];
            bool sr = (header & 0x10) != 0; // Short Record flag
            uint8_t tnf = header & 0x07;

            if (idx >= dataLen) break;
            uint8_t typeLen = data[idx++];

            size_t payloadLen = 0;
            if (sr) {
                if (idx >= dataLen) break;
                payloadLen = data[idx++];
            } else {
                if (idx + 3 >= dataLen) break;
                payloadLen = (data[idx] << 24) | (data[idx+1] << 16) | (data[idx+2] << 8) | data[idx+3];
                idx += 4;
            }

            // Skip Type Field
            idx += typeLen;

            // Extract Payload
            if (idx + payloadLen <= dataLen + 10 && payloadLen > 2) {
                char* buffer = (char*)malloc(payloadLen + 1);
                if (buffer) {
                    memcpy(buffer, &data[idx], payloadLen);
                    buffer[payloadLen] = '\0';
                    outJson = String(buffer);
                    free(buffer);
                    return true;
                }
            }
            break;
        } else if (data[idx] == 0x00) {
            // Null TLV / padding
            idx++;
        } else {
            idx++;
        }
    }

    // Method 2: Resilient Fallback - Scan for JSON delimiters '{' and '}'
    int firstBrace = -1;
    int lastBrace = -1;
    for (size_t i = 0; i < dataLen; i++) {
        if (data[i] == '{' && firstBrace == -1) {
            firstBrace = i;
        }
        if (data[i] == '}') {
            lastBrace = i;
        }
    }

    if (firstBrace != -1 && lastBrace != -1 && lastBrace > firstBrace) {
        size_t len = (lastBrace - firstBrace) + 1;
        char* buffer = (char*)malloc(len + 1);
        if (buffer) {
            memcpy(buffer, &data[firstBrace], len);
            buffer[len] = '\0';
            outJson = String(buffer);
            free(buffer);
            return true;
        }
    }

    return false;
}

size_t NfcManager::buildNdefPayload(const SpoolTag& tag, uint8_t* outBuffer, size_t maxLen) {
    String json = tag.toJSON();
    const char* mimeType = "application/json";
    uint8_t mimeLen = strlen(mimeType);
    size_t jsonLen = json.length();

    // Total NDEF Record Size:
    // 1 (Header 0xD2) + 1 (TypeLen) + 1 (PayloadLen) + mimeLen + jsonLen
    size_t ndefRecordLen = 1 + 1 + 1 + mimeLen + jsonLen;

    // Total TLV stream size:
    // 1 (TLV Tag 0x03) + 1 (Length) + ndefRecordLen + 1 (Terminator 0xFE)
    size_t totalTlvLen = 1 + 1 + ndefRecordLen + 1;

    if (totalTlvLen > maxLen) {
        return 0;
    }

    size_t idx = 0;
    // TLV Header
    outBuffer[idx++] = 0x03; // NDEF TLV Tag
    outBuffer[idx++] = (uint8_t)ndefRecordLen; // Length

    // NDEF Record
    outBuffer[idx++] = 0xD2; // MB=1, ME=1, SR=1, TNF=0x02 (MIME Media)
    outBuffer[idx++] = mimeLen; // 16
    outBuffer[idx++] = (uint8_t)jsonLen;
    
    // Type ("application/json")
    memcpy(&outBuffer[idx], mimeType, mimeLen);
    idx += mimeLen;

    // Payload (JSON string)
    memcpy(&outBuffer[idx], json.c_str(), jsonLen);
    idx += jsonLen;

    // Terminator TLV
    outBuffer[idx++] = 0xFE;

    // Pad remaining page bytes with 0x00 to complete 4-byte boundary
    while ((idx % 4) != 0) {
        outBuffer[idx++] = 0x00;
    }

    return idx;
}

bool NfcManager::writeTag(const SpoolTag& tag, String& outError) {
    if (!_initialized) {
        outError = "PN532 not initialized";
        return false;
    }

    AudioFeedback::setLedMode(LED_MODE_BUSY);

    while (_nfcSerial.available()) _nfcSerial.read();

    // 1. Detect Tag Presence
    uint8_t uid[7] = {0};
    uint8_t uidLen = 0;
    if (!_nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 500)) {
        outError = "No tag detected. Hold tag to reader.";
        AudioFeedback::playError();
        return false;
    }

    while (_nfcSerial.available()) _nfcSerial.read();
    delay(5);

    // 2. Prepare NDEF Buffer
    uint8_t ndefBuffer[512] = {0};
    size_t totalBytes = buildNdefPayload(tag, ndefBuffer, sizeof(ndefBuffer));
    if (totalBytes == 0) {
        outError = "Payload too large for buffer";
        AudioFeedback::playError();
        return false;
    }

    uint8_t totalPages = (totalBytes + 3) / 4;
    Serial.printf("[NFC] Writing %d bytes (%d pages) to NTAG215 starting at page 4...\n", totalBytes, totalPages);

    // 3. Write Pages to NTAG215 (starting at page 4)
    for (uint8_t p = 0; p < totalPages; p++) {
        uint8_t pageNum = 4 + p;
        while (_nfcSerial.available()) _nfcSerial.read();

        if (!_nfc.ntag2xx_WritePage(pageNum, &ndefBuffer[p * 4])) {
            outError = "Write error at page " + String(pageNum);
            AudioFeedback::playError();
            return false;
        }
        delay(8); // Small delay for EEPROM write cycle
    }

    // 4. Verify Written Data
    while (_nfcSerial.available()) _nfcSerial.read();
    delay(10);
    SpoolTag verifyTag;
    if (readTag(verifyTag)) {
        if (verifyTag.brand == tag.brand && verifyTag.type == tag.type && verifyTag.color_hex == tag.color_hex) {
            Serial.println("[NFC] Write & Verification SUCCESS!");
            AudioFeedback::playWriteSuccess();
            return true;
        }
    }

    outError = "Verification mismatch";
    AudioFeedback::playError();
    return false;
}
