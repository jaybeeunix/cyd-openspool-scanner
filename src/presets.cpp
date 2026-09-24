#include "presets.h"

static const FilamentPreset PRESETS[] = {
    {"Sunlu",     "PLA",         "FFFFFF", "White",          200, 230, 50, 60},
    {"Sunlu",     "PLA+",        "0077FF", "Blue",           210, 235, 55, 65},
    {"Sunlu",     "PETG",        "111111", "Black",          220, 250, 70, 85},
    {"Sunlu",     "ABS",         "EE2222", "Red",            240, 270, 85, 110},
    {"Sunlu",     "ASA",         "555555", "Dark Grey",      245, 270, 90, 110},
    {"Jayo",      "PLA",         "FFFFFF", "White",          200, 230, 50, 60},
    {"Jayo",      "PLA+",        "111111", "Black",          210, 235, 55, 65},
    {"Jayo",      "PETG",        "FF7700", "Orange",         220, 250, 70, 85},
    {"Jayo",      "ABS",         "111111", "Black",          240, 270, 85, 110},
    {"Jayo",      "ASA",         "D3D3D3", "Light Grey",     245, 270, 90, 110},
    {"Polymaker", "PolyLite PLA","EE2222", "Crimson Red",    195, 220, 50, 60},
    {"Polymaker", "PolyLite PLA","0077FF", "Electric Blue",  195, 220, 50, 60},
    {"Polymaker", "PolyTerra PLA","3B5360","Army Blue",      190, 215, 45, 60},
    {"eSun",      "PLA+",        "FFFFFF", "Cold White",     205, 225, 55, 65},
    {"eSun",      "PLA+",        "111111", "Deep Black",     205, 225, 55, 65},
    {"eSun",      "PETG",        "008080", "Solid Teal",     230, 250, 75, 85},
    {"Sunlu",     "PLA-Meta",    "FFD700", "Canary Yellow",  185, 205, 50, 60},
    {"Overture",  "TPU-95A",     "111111", "Flexible Black", 215, 235, 40, 50},
    {"Generic",   "ABS",         "EE2222", "Fire Red",       235, 260, 90, 100},
    {"Generic",   "ASA",         "444444", "Weather Grey",   240, 270, 95, 105},
    {"Generic",   "PA-CF",       "1E1E1E", "Nylon Carbon",   270, 300, 80, 100}
};

static const ColorItem COLORS[] = {
    {"FFFFFF", "White",       0xFFFF},
    {"111111", "Black",       0x1082},
    {"D3D3D3", "Light Grey",  0xD69A},
    {"555555", "Dark Grey",   0x52AA},
    {"EE2222", "Red",         0xF904},
    {"FF7700", "Orange",      0xFB80},
    {"FFD700", "Yellow",      0xFEC0},
    {"00AE42", "Green",       0x0568},
    {"008080", "Teal",        0x0410},
    {"0077FF", "Sky Blue",    0x03BF},
    {"0000CC", "Deep Blue",   0x0019},
    {"8800CC", "Purple",      0x8819},
    {"FF3388", "Pink",        0xF9B1},
    {"8B4513", "Brown",       0x8A22},
    {"C0C0C0", "Silver",      0xC618},
    {"3B5360", "Slate",       0x3A8C},
    {"8F9779", "Olive",       0x8CBF}
};

static const char* MATERIALS[] = {
    "PLA", "PLA+", "PLA-Silk", "PLA-CF", "PLA-Matte", 
    "PETG", "PETG-CF", "ABS", "ASA", 
    "TPU-95A", "TPU-90A", "PC", "PA-CF", "PVA"
};

static const char* BRANDS[] = {
    "Jayo", "Polymaker", "eSun", "Sunlu", 
    "Overture", "Elegoo", "Prusament", "Generic"
};

struct MaterialDefault {
    const char* type;
    int min_temp;
    int max_temp;
    int bed_min_temp;
    int bed_max_temp;
};

static const MaterialDefault MATERIAL_DEFAULTS[] = {
    {"PLA",        200, 230, 50, 60},
    {"PLA+",       210, 235, 55, 65},
    {"PLA-Silk",   205, 230, 50, 60},
    {"PLA-CF",     210, 240, 50, 60},
    {"PLA-Matte",  200, 230, 50, 60},
    {"PETG",       220, 250, 70, 85},
    {"PETG-CF",    230, 260, 75, 85},
    {"ABS",        240, 270, 85, 110},
    {"ASA",        245, 270, 90, 110},
    {"TPU-95A",    215, 235, 35, 50},
    {"TPU-90A",    210, 230, 30, 45},
    {"PC",         260, 290, 90, 115},
    {"PA-CF",      270, 300, 80, 100},
    {"PVA",        190, 215, 45, 60}
};

void PresetManager::init() {
    // Initializer if needed
}

size_t PresetManager::getPresetCount() {
    return sizeof(PRESETS) / sizeof(PRESETS[0]);
}

FilamentPreset PresetManager::getPreset(size_t index) {
    if (index < getPresetCount()) {
        return PRESETS[index];
    }
    return PRESETS[0];
}

SpoolTag PresetManager::createTagFromPreset(size_t index) {
    FilamentPreset p = getPreset(index);
    SpoolTag tag;
    tag.brand = p.brand;
    tag.type = p.type;
    tag.color_hex = p.color_hex;
    tag.min_temp = p.min_temp;
    tag.max_temp = p.max_temp;
    tag.bed_min_temp = p.bed_min_temp;
    tag.bed_max_temp = p.bed_max_temp;
    tag.weight = 1000;
    tag.valid = true;
    return tag;
}

size_t PresetManager::getColorCount() {
    return sizeof(COLORS) / sizeof(COLORS[0]);
}

ColorItem PresetManager::getColor(size_t index) {
    if (index < getColorCount()) {
        return COLORS[index];
    }
    return COLORS[0];
}

size_t PresetManager::getMaterialCount() {
    return sizeof(MATERIALS) / sizeof(MATERIALS[0]);
}

const char* PresetManager::getMaterial(size_t index) {
    if (index < getMaterialCount()) {
        return MATERIALS[index];
    }
    return MATERIALS[0];
}

size_t PresetManager::getBrandCount() {
    return sizeof(BRANDS) / sizeof(BRANDS[0]);
}

const char* PresetManager::getBrand(size_t index) {
    if (index < getBrandCount()) {
        return BRANDS[index];
    }
    return BRANDS[0];
}

void PresetManager::applyMaterialTemps(SpoolTag& tag) {
    // 1. Check if an existing preset matches Brand + Type
    for (size_t i = 0; i < sizeof(PRESETS) / sizeof(PRESETS[0]); i++) {
        if (tag.brand.equalsIgnoreCase(PRESETS[i].brand) && tag.type.equalsIgnoreCase(PRESETS[i].type)) {
            tag.min_temp = PRESETS[i].min_temp;
            tag.max_temp = PRESETS[i].max_temp;
            tag.bed_min_temp = PRESETS[i].bed_min_temp;
            tag.bed_max_temp = PRESETS[i].bed_max_temp;
            return;
        }
    }

    // 2. Check if an existing preset matches Type
    for (size_t i = 0; i < sizeof(PRESETS) / sizeof(PRESETS[0]); i++) {
        if (tag.type.equalsIgnoreCase(PRESETS[i].type)) {
            tag.min_temp = PRESETS[i].min_temp;
            tag.max_temp = PRESETS[i].max_temp;
            tag.bed_min_temp = PRESETS[i].bed_min_temp;
            tag.bed_max_temp = PRESETS[i].bed_max_temp;
            return;
        }
    }

    // 3. Fallback to material defaults
    for (size_t i = 0; i < sizeof(MATERIAL_DEFAULTS) / sizeof(MATERIAL_DEFAULTS[0]); i++) {
        if (tag.type.equalsIgnoreCase(MATERIAL_DEFAULTS[i].type)) {
            tag.min_temp = MATERIAL_DEFAULTS[i].min_temp;
            tag.max_temp = MATERIAL_DEFAULTS[i].max_temp;
            tag.bed_min_temp = MATERIAL_DEFAULTS[i].bed_min_temp;
            tag.bed_max_temp = MATERIAL_DEFAULTS[i].bed_max_temp;
            return;
        }
    }
}
