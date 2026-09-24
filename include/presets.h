#pragma once

#include "openspool_models.h"
#include <vector>

struct FilamentPreset {
    const char* brand;
    const char* type;
    const char* color_hex;
    const char* color_name;
    int min_temp;
    int max_temp;
    int bed_min_temp;
    int bed_max_temp;
};

struct ColorItem {
    const char* hex;
    const char* name;
    uint16_t rgb565;
};

class PresetManager {
public:
    static void init();
    static size_t getPresetCount();
    static FilamentPreset getPreset(size_t index);
    static SpoolTag createTagFromPreset(size_t index);

    static size_t getColorCount();
    static ColorItem getColor(size_t index);

    static size_t getMaterialCount();
    static const char* getMaterial(size_t index);

    static size_t getBrandCount();
    static const char* getBrand(size_t index);

    static void applyMaterialTemps(SpoolTag& tag);
};
