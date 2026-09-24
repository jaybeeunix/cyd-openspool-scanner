#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"
#include "openspool_models.h"
#include "presets.h"

enum ScreenState {
    SCREEN_DASHBOARD,
    SCREEN_PRESETS,
    SCREEN_EDITOR,
    SCREEN_CLONER,
    SCREEN_RAW_VIEW
};

struct TouchPoint {
    int16_t x;
    int16_t y;
    bool pressed;
};

class UIManager {
public:
    UIManager();
    void init();

    // Redraw screen when state changes or data updates
    void update(bool forceRedraw = false);

    // Process touch events
    void handleTouch();

    // Screen navigation
    void setScreen(ScreenState state);
    ScreenState getScreen() const { return _currentScreen; }

    // Tag data setters/getters
    void setCurrentTag(const SpoolTag& tag);
    SpoolTag getCurrentTag() const { return _currentTag; }
    void setEditingTag(const SpoolTag& tag);
    SpoolTag getEditingTag() const { return _editingTag; }

    // Status notifications (toasts / banners)
    void showStatusMessage(const String& msg, uint16_t color = TFT_WHITE, uint32_t duration_ms = 2500);

    // Call when write action is requested
    void setWriteRequestedCallback(void (*callback)(const SpoolTag& tag)) {
        _onWriteRequested = callback;
    }

private:
    TFT_eSPI _tft;
    SPIClass _touchSPI;
    XPT2046_Touchscreen _touch;
    
    ScreenState _currentScreen;
    ScreenState _lastRenderedScreen;
    SpoolTag _currentTag;
    SpoolTag _editingTag;
    SpoolTag _cloneSourceTag;

    // Preset & Editor State
    size_t _presetPage;
    size_t _editBrandIdx;
    size_t _editMaterialIdx;
    size_t _editColorIdx;
    int _cloneCount;

    // Toast Message
    String _statusMessage;
    uint16_t _statusColor;
    unsigned long _statusExpiry;

    // Touch debouncing
    bool _wasTouched;
    unsigned long _lastTouchTime;

    void (*_onWriteRequested)(const SpoolTag& tag);

    TouchPoint getTouchPoint();

    // Drawing helpers
    void drawHeader(const char* title, bool showBackBtn = false);
    void drawFooter();
    void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t bgColor, uint16_t textColor, uint8_t textSize = 2);

    // Screen Renderers
    void renderDashboard();
    void renderPresets();
    void renderEditor();
    void renderCloner();
    void renderRawView();

    // Touch Dispatchers
    void handleDashboardTouch(int16_t tx, int16_t ty);
    void handlePresetsTouch(int16_t tx, int16_t ty);
    void handleEditorTouch(int16_t tx, int16_t ty);
    void handleClonerTouch(int16_t tx, int16_t ty);
};
