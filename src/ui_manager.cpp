#include "ui_manager.h"
#include "audio_feedback.h"

// Color Palette Constants
#define COLOR_BG          0x0842  // Dark Navy/Slate background
#define COLOR_CARD        0x18E4  // Card background
#define COLOR_CARD_BORDER 0x31A6  // Border grey
#define COLOR_ACCENT      0x05E0  // Emerald Green
#define COLOR_PRIMARY     0x041F  // Modern Blue
#define COLOR_WARN        0xFD20  // Amber Orange
#define COLOR_DANGER      0xF800  // Crimson Red
#define COLOR_TEXT_MUTED  0x9CD3  // Light Slate Grey

UIManager::UIManager() 
    : _tft(),
      _touchSPI(VSPI),
      _touch(XPT2046_CS, XPT2046_IRQ),
      _currentScreen(SCREEN_DASHBOARD),
      _lastRenderedScreen((ScreenState)-1),
      _presetPage(0),
      _editBrandIdx(0),
      _editMaterialIdx(0),
      _editColorIdx(0),
      _cloneCount(0),
      _statusColor(TFT_WHITE),
      _statusExpiry(0),
      _wasTouched(false),
      _lastTouchTime(0),
      _onWriteRequested(nullptr) {
    _editingTag = PresetManager::createTagFromPreset(0);
    _cloneSourceTag = _editingTag;
}

void UIManager::init() {
    // Backlight initialization
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);

    // Initialize TFT Display in Portrait Mode (240x320)
    _tft.init();
    _tft.setRotation(SCREEN_ROTATION);
    _tft.fillScreen(COLOR_BG);

    // Initialize Touch Controller on Dedicated SPI bus
    _touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    _touch.begin(_touchSPI);
    _touch.setRotation(SCREEN_ROTATION);

    showStatusMessage("Ready to Scan", TFT_GREEN, 2000);
    update(true);
}

void UIManager::setScreen(ScreenState state) {
    if (_currentScreen != state) {
        _currentScreen = state;
        if (state == SCREEN_EDITOR) {
            // Synchronize material index with current editing tag
            size_t matCount = PresetManager::getMaterialCount();
            for (size_t i = 0; i < matCount; i++) {
                if (_editingTag.type.equalsIgnoreCase(PresetManager::getMaterial(i))) {
                    _editMaterialIdx = i;
                    break;
                }
            }
            // Synchronize brand index with current editing tag
            size_t brandCount = PresetManager::getBrandCount();
            for (size_t i = 0; i < brandCount; i++) {
                if (_editingTag.brand.equalsIgnoreCase(PresetManager::getBrand(i))) {
                    _editBrandIdx = i;
                    break;
                }
            }
        }
        update(true);
    }
}

void UIManager::setCurrentTag(const SpoolTag& tag) {
    _currentTag = tag;
    if (_currentScreen == SCREEN_DASHBOARD || _currentScreen == SCREEN_CLONER) {
        update(true);
    }
}

void UIManager::setEditingTag(const SpoolTag& tag) {
    _editingTag = tag;
    _cloneSourceTag = tag;
}

void UIManager::showStatusMessage(const String& msg, uint16_t color, uint32_t duration_ms) {
    _statusMessage = msg;
    _statusColor = color;
    _statusExpiry = millis() + duration_ms;
    drawFooter();
}

TouchPoint UIManager::getTouchPoint() {
    TouchPoint tp = {0, 0, false};
    if (_touch.touched()) {
        TS_Point p = _touch.getPoint();
        if (p.z > 200) { // Valid pressure
            // Map raw touch ADC readings to screen pixels (240x320)
            int16_t x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_WIDTH);
            int16_t y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_HEIGHT);
            tp.x = constrain(x, 0, SCREEN_WIDTH - 1);
            tp.y = constrain(y, 0, SCREEN_HEIGHT - 1);
            tp.pressed = true;
        }
    }
    return tp;
}

void UIManager::handleTouch() {
    TouchPoint tp = getTouchPoint();
    unsigned long now = millis();

    if (tp.pressed) {
        if (!_wasTouched && (now - _lastTouchTime > 180)) {
            _wasTouched = true;
            _lastTouchTime = now;
            AudioFeedback::playTouchClick();

            // Dispatch based on active screen
            switch (_currentScreen) {
                case SCREEN_DASHBOARD:
                    handleDashboardTouch(tp.x, tp.y);
                    break;
                case SCREEN_PRESETS:
                    handlePresetsTouch(tp.x, tp.y);
                    break;
                case SCREEN_EDITOR:
                    handleEditorTouch(tp.x, tp.y);
                    break;
                case SCREEN_CLONER:
                    handleClonerTouch(tp.x, tp.y);
                    break;
                case SCREEN_RAW_VIEW:
                    setScreen(SCREEN_DASHBOARD);
                    break;
            }
        }
    } else {
        _wasTouched = false;
    }
}

void UIManager::update(bool forceRedraw) {
    if (forceRedraw || _currentScreen != _lastRenderedScreen) {
        _tft.fillScreen(COLOR_BG);
        _lastRenderedScreen = _currentScreen;

        switch (_currentScreen) {
            case SCREEN_DASHBOARD:
                renderDashboard();
                break;
            case SCREEN_PRESETS:
                renderPresets();
                break;
            case SCREEN_EDITOR:
                renderEditor();
                break;
            case SCREEN_CLONER:
                renderCloner();
                break;
            case SCREEN_RAW_VIEW:
                renderRawView();
                break;
        }
    }

    // Refresh footer if status active or expired
    if (_statusExpiry > 0 && millis() > _statusExpiry) {
        _statusExpiry = 0;
        _statusMessage = "";
        drawFooter();
    }
}

void UIManager::drawHeader(const char* title, bool showBackBtn) {
    _tft.fillRect(0, 0, SCREEN_WIDTH, 32, 0x10A2);
    _tft.drawFastHLine(0, 32, SCREEN_WIDTH, COLOR_CARD_BORDER);

    if (showBackBtn) {
        drawButton(6, 4, 52, 24, "< BACK", COLOR_CARD, TFT_WHITE, 1);
        _tft.setTextColor(TFT_WHITE, 0x10A2);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString(title, 148, 16, 2);
    } else {
        _tft.setTextColor(TFT_CYAN, 0x10A2);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString(title, SCREEN_WIDTH / 2, 16, 2);
    }
}

void UIManager::drawFooter() {
    _tft.fillRect(0, SCREEN_HEIGHT - 22, SCREEN_WIDTH, 22, 0x0821);
    _tft.drawFastHLine(0, SCREEN_HEIGHT - 22, SCREEN_WIDTH, COLOR_CARD_BORDER);
    _tft.setTextDatum(MC_DATUM);

    if (_statusMessage.length() > 0) {
        _tft.setTextColor(_statusColor, 0x0821);
        _tft.drawString(_statusMessage.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 11, 2);
    } else {
        _tft.setTextColor(COLOR_TEXT_MUTED, 0x0821);
        _tft.drawString("CYD OpenSpool v" APP_VERSION, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 11, 1);
    }
}

void UIManager::drawButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t bgColor, uint16_t textColor, uint8_t textSize) {
    _tft.fillRoundRect(x, y, w, h, 5, bgColor);
    _tft.drawRoundRect(x, y, w, h, 5, COLOR_CARD_BORDER);
    _tft.setTextColor(textColor, bgColor);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(label, x + (w / 2), y + (h / 2), textSize);
}

// =============================================================================
// SCREEN 1: DASHBOARD / SCANNER VIEW
// =============================================================================
void UIManager::renderDashboard() {
    drawHeader("OpenSpool Scanner", false);

    if (_currentTag.valid) {
        // 1. Color Preview Swatch Card
        uint16_t swatchColor = _currentTag.getRGB565Color();
        uint16_t textColor = _currentTag.getContrastingTextColor();
        _tft.fillRoundRect(12, 38, 216, 54, 6, swatchColor);
        _tft.drawRoundRect(12, 38, 216, 54, 6, TFT_WHITE);
        
        _tft.setTextColor(textColor, swatchColor);
        _tft.setTextDatum(MC_DATUM);
        String hexTitle = "#" + _currentTag.color_hex;
        _tft.drawString(hexTitle.c_str(), 120, 56, 4);
        _tft.drawString(_currentTag.type.c_str(), 120, 78, 2);

        // 2. Spool Details Card
        _tft.fillRoundRect(12, 98, 216, 120, 6, COLOR_CARD);
        _tft.drawRoundRect(12, 98, 216, 120, 6, COLOR_CARD_BORDER);

        _tft.setTextDatum(TL_DATUM);
        _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
        _tft.drawString("Brand:", 20, 106, 2);
        _tft.drawString("Material:", 20, 126, 2);
        _tft.drawString("Nozzle:", 20, 146, 2);
        _tft.drawString("Bed:", 20, 166, 2);
        _tft.drawString("UID:", 20, 186, 1);

        _tft.setTextColor(TFT_WHITE, COLOR_CARD);
        _tft.drawString(_currentTag.brand.c_str(), 85, 106, 2);
        _tft.drawString(_currentTag.type.c_str(), 85, 126, 2);
        
        String nozzleStr = String(_currentTag.min_temp) + " - " + String(_currentTag.max_temp) + " C";
        _tft.drawString(nozzleStr.c_str(), 85, 146, 2);

        String bedStr = String(_currentTag.bed_min_temp) + " - " + String(_currentTag.bed_max_temp) + " C";
        _tft.drawString(bedStr.c_str(), 85, 166, 2);

        _tft.drawString(_currentTag.uid_str.c_str(), 85, 186, 1);
    } else {
        // No Tag Detected - Animated Radar / Prompt Card
        _tft.fillRoundRect(12, 38, 216, 180, 8, COLOR_CARD);
        _tft.drawRoundRect(12, 38, 216, 180, 8, COLOR_CARD_BORDER);

        // NFC Antenna Graphic Ring
        _tft.drawCircle(120, 105, 36, TFT_CYAN);
        _tft.drawCircle(120, 105, 26, 0x03BF);
        _tft.drawCircle(120, 105, 16, 0x018F);

        _tft.setTextColor(TFT_WHITE, COLOR_CARD);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("Ready to Scan", 120, 155, 4);
        
        _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
        _tft.drawString("Hold Spool Tag to Reader", 120, 182, 2);
    }

    // Action Buttons at Bottom
    drawButton(12, 226, 216, 32, "EDIT / CONFIGURE", COLOR_PRIMARY, TFT_WHITE, 2);
    drawButton(12, 264, 104, 30, "PRESETS", 0x2124, TFT_WHITE, 2);
    drawButton(124, 264, 104, 30, "CLONE", COLOR_WARN, TFT_BLACK, 2);

    drawFooter();
}

void UIManager::handleDashboardTouch(int16_t tx, int16_t ty) {
    // Edit / Configure button (12, 226, 216, 32)
    if (tx >= 12 && tx <= 228 && ty >= 226 && ty <= 258) {
        if (_currentTag.valid) {
            _editingTag = _currentTag;
        }
        setScreen(SCREEN_EDITOR);
        return;
    }

    // Presets button (12, 264, 104, 30)
    if (tx >= 12 && tx <= 116 && ty >= 264 && ty <= 294) {
        setScreen(SCREEN_PRESETS);
        return;
    }

    // Clone button (124, 264, 104, 30)
    if (tx >= 124 && tx <= 228 && ty >= 264 && ty <= 294) {
        if (_currentTag.valid) {
            _cloneSourceTag = _currentTag;
        }
        _cloneCount = 0;
        setScreen(SCREEN_CLONER);
        return;
    }

    // Tap on Spool Details card -> Open Raw View
    if (_currentTag.valid && tx >= 12 && tx <= 228 && ty >= 38 && ty <= 218) {
        setScreen(SCREEN_RAW_VIEW);
        return;
    }
}

// =============================================================================
// SCREEN 2: PRESETS LIBRARY
// =============================================================================
void UIManager::renderPresets() {
    drawHeader("Filament Presets", true);

    const size_t ITEMS_PER_PAGE = 4;
    size_t total = PresetManager::getPresetCount();
    size_t startIdx = _presetPage * ITEMS_PER_PAGE;
    size_t endIdx = min(startIdx + ITEMS_PER_PAGE, total);

    for (size_t i = startIdx; i < endIdx; i++) {
        size_t row = i - startIdx;
        int16_t y = 38 + (row * 56);
        FilamentPreset p = PresetManager::getPreset(i);

        _tft.fillRoundRect(10, y, 220, 50, 5, COLOR_CARD);
        _tft.drawRoundRect(10, y, 220, 50, 5, COLOR_CARD_BORDER);

        // Color circle
        SpoolTag tmpTag;
        tmpTag.color_hex = p.color_hex;
        uint16_t col = tmpTag.getRGB565Color();
        _tft.fillCircle(28, y + 25, 14, col);
        _tft.drawCircle(28, y + 25, 14, TFT_WHITE);

        // Details
        _tft.setTextDatum(TL_DATUM);
        _tft.setTextColor(TFT_WHITE, COLOR_CARD);
        String title = String(p.brand) + " " + String(p.type);
        _tft.drawString(title.c_str(), 48, y + 8, 2);

        _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
        String subtitle = String(p.color_name) + " (" + String(p.min_temp) + "-" + String(p.max_temp) + "C)";
        _tft.drawString(subtitle.c_str(), 48, y + 28, 1);
    }

    // Pagination Controls
    drawButton(10, 266, 68, 28, "< PREV", COLOR_CARD, TFT_WHITE, 1);
    
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d / %d", (int)(_presetPage + 1), (int)((total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE));
    _tft.setTextColor(TFT_WHITE, COLOR_BG);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(pageStr, 120, 280, 2);

    drawButton(162, 266, 68, 28, "NEXT >", COLOR_CARD, TFT_WHITE, 1);

    drawFooter();
}

void UIManager::handlePresetsTouch(int16_t tx, int16_t ty) {
    // Back button in header
    if (tx <= 60 && ty <= 32) {
        setScreen(SCREEN_DASHBOARD);
        return;
    }

    const size_t ITEMS_PER_PAGE = 4;
    size_t total = PresetManager::getPresetCount();

    // Prev Button
    if (tx >= 10 && tx <= 78 && ty >= 266 && ty <= 294) {
        if (_presetPage > 0) {
            _presetPage--;
            update(true);
        }
        return;
    }

    // Next Button
    if (tx >= 162 && tx <= 230 && ty >= 266 && ty <= 294) {
        if ((_presetPage + 1) * ITEMS_PER_PAGE < total) {
            _presetPage++;
            update(true);
        }
        return;
    }

    // Preset Item Taps
    size_t startIdx = _presetPage * ITEMS_PER_PAGE;
    for (size_t r = 0; r < ITEMS_PER_PAGE; r++) {
        int16_t y = 38 + (r * 56);
        if (ty >= y && ty <= y + 50 && tx >= 10 && tx <= 230) {
            size_t selIdx = startIdx + r;
            if (selIdx < total) {
                _editingTag = PresetManager::createTagFromPreset(selIdx);
                _cloneSourceTag = _editingTag;
                setScreen(SCREEN_EDITOR);
            }
            return;
        }
    }
}

// =============================================================================
// SCREEN 3: SPOOL EDITOR & BURNER
// =============================================================================
void UIManager::renderEditor() {
    drawHeader("Configure Tag", true);

    // 1. Material Selector Row
    _tft.fillRoundRect(10, 36, 220, 32, 4, COLOR_CARD);
    drawButton(14, 40, 28, 24, "<", 0x2945, TFT_WHITE, 2);
    _tft.setTextColor(TFT_WHITE, COLOR_CARD);
    _tft.setTextDatum(MC_DATUM);
    String matStr = "Mat: " + _editingTag.type;
    _tft.drawString(matStr.c_str(), 120, 52, 2);
    drawButton(198, 40, 28, 24, ">", 0x2945, TFT_WHITE, 2);

    // 2. Brand Selector Row
    _tft.fillRoundRect(10, 72, 220, 32, 4, COLOR_CARD);
    drawButton(14, 76, 28, 24, "<", 0x2945, TFT_WHITE, 2);
    _tft.setTextColor(TFT_WHITE, COLOR_CARD);
    _tft.setTextDatum(MC_DATUM);
    String brandStr = "Brand: " + _editingTag.brand;
    _tft.drawString(brandStr.c_str(), 120, 88, 2);
    drawButton(198, 76, 28, 24, ">", 0x2945, TFT_WHITE, 2);

    // 3. Color Palette Grid
    _tft.fillRoundRect(10, 108, 220, 58, 4, COLOR_CARD);
    size_t totalColors = PresetManager::getColorCount();
    size_t colCount = (totalColors > 16) ? 9 : 8;
    int spacing = (totalColors > 16) ? 23 : 26;
    for (size_t i = 0; i < totalColors && i < 18; i++) {
        ColorItem c = PresetManager::getColor(i);
        int col = i % colCount;
        int row = i / colCount;
        int16_t cx = 22 + (col * spacing);
        int16_t cy = 122 + (row * 28);
        _tft.fillCircle(cx, cy, 10, c.rgb565);
        if (_editingTag.color_hex.equalsIgnoreCase(c.hex)) {
            _tft.drawCircle(cx, cy, 12, TFT_WHITE);
            _tft.drawCircle(cx, cy, 11, TFT_BLACK);
        }
    }

    // 4. Nozzle & Bed Temperature Steppers
    _tft.fillRoundRect(10, 170, 220, 38, 4, COLOR_CARD);
    drawButton(14, 174, 30, 30, "-", 0x2945, TFT_WHITE, 2);
    _tft.setTextColor(TFT_WHITE, COLOR_CARD);
    _tft.setTextDatum(MC_DATUM);
    String nozStr = "Noz: " + String(_editingTag.min_temp) + "-" + String(_editingTag.max_temp) + " C";
    _tft.drawString(nozStr.c_str(), 120, 189, 2);
    drawButton(196, 174, 30, 30, "+", 0x2945, TFT_WHITE, 2);

    _tft.fillRoundRect(10, 212, 220, 38, 4, COLOR_CARD);
    drawButton(14, 216, 30, 30, "-", 0x2945, TFT_WHITE, 2);
    _tft.setTextColor(TFT_WHITE, COLOR_CARD);
    _tft.setTextDatum(MC_DATUM);
    String bedStr = "Bed: " + String(_editingTag.bed_min_temp) + "-" + String(_editingTag.bed_max_temp) + " C";
    _tft.drawString(bedStr.c_str(), 120, 231, 2);
    drawButton(196, 216, 30, 30, "+", 0x2945, TFT_WHITE, 2);

    // 5. Big Burn Button
    drawButton(10, 256, 220, 38, "BURN TO TAG (WRITE)", COLOR_ACCENT, TFT_BLACK, 2);

    drawFooter();
}

void UIManager::handleEditorTouch(int16_t tx, int16_t ty) {
    // Back button
    if (tx <= 60 && ty <= 32) {
        setScreen(SCREEN_DASHBOARD);
        return;
    }

    // Material Prev/Next
    if (ty >= 36 && ty <= 68) {
        size_t matCount = PresetManager::getMaterialCount();
        if (tx >= 14 && tx <= 42) { // Prev
            _editMaterialIdx = (_editMaterialIdx + matCount - 1) % matCount;
            _editingTag.type = PresetManager::getMaterial(_editMaterialIdx);
            PresetManager::applyMaterialTemps(_editingTag);
            update(true);
            return;
        } else if (tx >= 198 && tx <= 226) { // Next
            _editMaterialIdx = (_editMaterialIdx + 1) % matCount;
            _editingTag.type = PresetManager::getMaterial(_editMaterialIdx);
            PresetManager::applyMaterialTemps(_editingTag);
            update(true);
            return;
        }
    }

    // Brand Prev/Next
    if (ty >= 72 && ty <= 104) {
        size_t brandCount = PresetManager::getBrandCount();
        if (tx >= 14 && tx <= 42) {
            _editBrandIdx = (_editBrandIdx + brandCount - 1) % brandCount;
            _editingTag.brand = PresetManager::getBrand(_editBrandIdx);
            PresetManager::applyMaterialTemps(_editingTag);
            update(true);
            return;
        } else if (tx >= 198 && tx <= 226) {
            _editBrandIdx = (_editBrandIdx + 1) % brandCount;
            _editingTag.brand = PresetManager::getBrand(_editBrandIdx);
            PresetManager::applyMaterialTemps(_editingTag);
            update(true);
            return;
        }
    }

    // Color Palette Taps (108 to 166)
    if (ty >= 108 && ty <= 166 && tx >= 10 && tx <= 230) {
        size_t totalColors = PresetManager::getColorCount();
        size_t colCount = (totalColors > 16) ? 9 : 8;
        int spacing = (totalColors > 16) ? 23 : 26;
        int col = (tx - 10) / spacing;
        int row = (ty - 110) / 28;
        col = constrain(col, 0, (int)colCount - 1);
        row = constrain(row, 0, 1);
        size_t idx = row * colCount + col;
        if (idx < totalColors) {
            _editingTag.color_hex = PresetManager::getColor(idx).hex;
            update(true);
            return;
        }
    }

    // Nozzle Temp Stepper
    if (ty >= 170 && ty <= 208) {
        if (tx >= 14 && tx <= 44) {
            _editingTag.min_temp = max(160, _editingTag.min_temp - 5);
            _editingTag.max_temp = max(_editingTag.min_temp, _editingTag.max_temp - 5);
            update(true);
            return;
        } else if (tx >= 196 && tx <= 226) {
            _editingTag.min_temp = min(320, _editingTag.min_temp + 5);
            _editingTag.max_temp = min(350, _editingTag.max_temp + 5);
            update(true);
            return;
        }
    }

    // Bed Temp Stepper
    if (ty >= 212 && ty <= 250) {
        if (tx >= 14 && tx <= 44) {
            _editingTag.bed_min_temp = max(20, _editingTag.bed_min_temp - 5);
            _editingTag.bed_max_temp = max(_editingTag.bed_min_temp, _editingTag.bed_max_temp - 5);
            update(true);
            return;
        } else if (tx >= 196 && tx <= 226) {
            _editingTag.bed_min_temp = min(120, _editingTag.bed_min_temp + 5);
            _editingTag.bed_max_temp = min(130, _editingTag.bed_max_temp + 5);
            update(true);
            return;
        }
    }

    // Burn To Tag Button (10, 256, 220, 38)
    if (tx >= 10 && tx <= 230 && ty >= 256 && ty <= 294) {
        if (_onWriteRequested) {
            showStatusMessage("Writing... Hold Tag Steady", TFT_YELLOW, 5000);
            _onWriteRequested(_editingTag);
        }
        return;
    }
}

// =============================================================================
// SCREEN 4: TAG CLONER
// =============================================================================
void UIManager::renderCloner() {
    drawHeader("Batch Cloner", true);

    // Source Tag Specs Card
    _tft.fillRoundRect(10, 38, 220, 110, 6, COLOR_CARD);
    _tft.drawRoundRect(10, 38, 220, 110, 6, COLOR_CARD_BORDER);

    uint16_t col = _cloneSourceTag.getRGB565Color();
    _tft.fillCircle(32, 65, 16, col);
    _tft.drawCircle(32, 65, 16, TFT_WHITE);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(TFT_WHITE, COLOR_CARD);
    String title = _cloneSourceTag.brand + " " + _cloneSourceTag.type;
    _tft.drawString(title.c_str(), 56, 52, 2);

    _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD);
    String hexStr = "Color: #" + _cloneSourceTag.color_hex;
    _tft.drawString(hexStr.c_str(), 56, 72, 2);

    String tempStr = "Nozzle: " + String(_cloneSourceTag.min_temp) + "-" + String(_cloneSourceTag.max_temp) + "C";
    _tft.drawString(tempStr.c_str(), 20, 96, 2);

    String bedStr = "Bed: " + String(_cloneSourceTag.bed_min_temp) + "-" + String(_cloneSourceTag.bed_max_temp) + "C";
    _tft.drawString(bedStr.c_str(), 20, 116, 2);

    // Counter Card
    _tft.fillRoundRect(10, 156, 220, 72, 6, 0x10A2);
    _tft.drawRoundRect(10, 156, 220, 72, 6, COLOR_CARD_BORDER);

    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(COLOR_TEXT_MUTED, 0x10A2);
    _tft.drawString("SUCCESSFULLY CLONED", 120, 175, 2);

    char countStr[16];
    snprintf(countStr, sizeof(countStr), "%d Tags", _cloneCount);
    _tft.setTextColor(TFT_GREEN, 0x10A2);
    _tft.drawString(countStr, 120, 204, 4);

    // Action / Prompt
    drawButton(10, 240, 220, 48, "BURN CURRENT TAG", COLOR_WARN, TFT_BLACK, 2);

    drawFooter();
}

void UIManager::handleClonerTouch(int16_t tx, int16_t ty) {
    if (tx <= 60 && ty <= 32) {
        setScreen(SCREEN_DASHBOARD);
        return;
    }

    if (tx >= 10 && tx <= 230 && ty >= 240 && ty <= 288) {
        if (_onWriteRequested) {
            showStatusMessage("Cloning... Hold Tag Steady", TFT_YELLOW, 5000);
            _onWriteRequested(_cloneSourceTag);
            _cloneCount++;
            update(true);
        }
        return;
    }
}

// =============================================================================
// SCREEN 5: RAW NDEF & DIAGNOSTICS
// =============================================================================
void UIManager::renderRawView() {
    drawHeader("Raw Diagnostics", true);

    _tft.fillRoundRect(8, 38, 224, 250, 4, 0x0000);
    _tft.drawRoundRect(8, 38, 224, 250, 4, COLOR_CARD_BORDER);

    _tft.setTextDatum(TL_DATUM);
    _tft.setTextColor(TFT_GREEN, 0x0000);
    _tft.drawString("[OpenSpool JSON]", 14, 46, 2);

    String json = _currentTag.toJSON();
    // Word wrap display
    int y = 70;
    int lineLen = 24;
    for (size_t i = 0; i < json.length(); i += lineLen) {
        String chunk = json.substring(i, min(i + lineLen, json.length()));
        _tft.drawString(chunk.c_str(), 14, y, 1);
        y += 14;
        if (y > 270) break;
    }

    drawFooter();
}
