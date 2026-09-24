#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct SpoolTag {
  String protocol;  // "openspool"
  String version;   // "1.0"
  String brand;     // e.g., "Bambu Lab", "Polymaker", "eSun", "Generic"
  String type;      // e.g., "PLA", "PETG", "ABS", "TPU-95A", "PA-CF"
  String color_hex; // 6-digit hex string (e.g., "FF5500")
  int min_temp;     // Nozzle Min Temp (°C)
  int max_temp;     // Nozzle Max Temp (°C)
  int bed_min_temp; // Bed Min Temp (°C)
  int bed_max_temp; // Bed Max Temp (°C)
  int weight;       // Weight in grams (e.g., 1000)
  String spool_id;  // Unique identifier (optional)
  String uid_str;   // Tag Hardware UID (e.g. "04:A2:3F:8C:91:00:80")
  bool valid;

  SpoolTag() { reset(); }

  void reset() {
    protocol = "openspool";
    version = "1.0";
    brand = "Generic";
    type = "PLA";
    color_hex = "00AE42";
    min_temp = 200;
    max_temp = 220;
    bed_min_temp = 50;
    bed_max_temp = 60;
    weight = 1000;
    spool_id = "";
    uid_str = "";
    valid = false;
  }

  // Convert 6-character hex (RRGGBB) to 16-bit RGB565 for TFT_eSPI
  uint16_t getRGB565Color() const {
    if (color_hex.length() < 6)
      return 0xFFFF; // Default white
    long number = strtol(color_hex.c_str(), NULL, 16);
    uint8_t r = (number >> 16) & 0xFF;
    uint8_t g = (number >> 8) & 0xFF;
    uint8_t b = number & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  // Calculate perceived luminance to choose black or white text for contrast
  uint16_t getContrastingTextColor() const {
    if (color_hex.length() < 6)
      return 0x0000;
    long number = strtol(color_hex.c_str(), NULL, 16);
    uint8_t r = (number >> 16) & 0xFF;
    uint8_t g = (number >> 8) & 0xFF;
    uint8_t b = number & 0xFF;
    // Standard Rec. 601 luminance
    int y = (r * 299 + g * 587 + b * 114) / 1000;
    return (y > 140) ? 0x0000 : 0xFFFF; // Black if bright, White if dark
  }

  // Serialize to OpenSpool 1.0 JSON string
  String toJSON() const {
    JsonDocument doc;
    doc["protocol"] = protocol.length() > 0 ? protocol : "openspool";
    doc["version"] = version.length() > 0 ? version : "1.0";
    doc["brand"] = brand;
    doc["type"] = type;
    doc["color_hex"] = color_hex;
    doc["min_temp"] = String(min_temp);
    doc["max_temp"] = String(max_temp);
    if (bed_min_temp > 0)
      doc["bed_min_temp"] = String(bed_min_temp);
    if (bed_max_temp > 0)
      doc["bed_max_temp"] = String(bed_max_temp);
    if (weight > 0)
      doc["weight"] = String(weight);
    if (spool_id.length() > 0)
      doc["spool_id"] = spool_id;

    String output;
    serializeJson(doc, output);
    return output;
  }

  // Parse OpenSpool JSON into struct
  bool fromJSON(const String &jsonStr) {
    if (jsonStr.length() == 0)
      return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) {
      return false;
    }

    protocol = doc["protocol"] | "openspool";
    version = doc["version"] | "1.0";
    brand = doc["brand"] | "Generic";
    type = doc["type"] | "PLA";
    color_hex = doc["color_hex"] | "FFFFFF";

    // Handle temperatures whether stored as integer or string
    if (doc["min_temp"].is<int>()) {
      min_temp = doc["min_temp"].as<int>();
    } else if (doc["min_temp"].is<const char *>()) {
      min_temp = atoi(doc["min_temp"].as<const char *>());
    } else {
      min_temp = 200;
    }

    if (doc["max_temp"].is<int>()) {
      max_temp = doc["max_temp"].as<int>();
    } else if (doc["max_temp"].is<const char *>()) {
      max_temp = atoi(doc["max_temp"].as<const char *>());
    } else {
      max_temp = 220;
    }

    if (doc["bed_min_temp"].is<int>()) {
      bed_min_temp = doc["bed_min_temp"].as<int>();
    } else if (doc["bed_min_temp"].is<const char *>()) {
      bed_min_temp = atoi(doc["bed_min_temp"].as<const char *>());
    } else {
      bed_min_temp = 50;
    }

    if (doc["bed_max_temp"].is<int>()) {
      bed_max_temp = doc["bed_max_temp"].as<int>();
    } else if (doc["bed_max_temp"].is<const char *>()) {
      bed_max_temp = atoi(doc["bed_max_temp"].as<const char *>());
    } else {
      bed_max_temp = 60;
    }

    if (doc["weight"].is<int>()) {
      weight = doc["weight"].as<int>();
    } else if (doc["weight"].is<const char *>()) {
      weight = atoi(doc["weight"].as<const char *>());
    } else {
      weight = 1000;
    }

    spool_id = doc["spool_id"] | "";
    valid = true;
    return true;
  }
};
