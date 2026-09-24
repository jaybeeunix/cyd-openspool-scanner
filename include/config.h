#pragma once

#include <Arduino.h>

#define APP_VERSION "1.1"

// =============================================================================
// Display & Touch Configuration (Portrait 240x320)
// =============================================================================
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define SCREEN_ROTATION                                                        \
  0 // 0: Portrait (USB at bottom), 2: Portrait (USB at top)

// CYD XPT2046 Touch Pins (Dedicated Touch SPI Bus)
#define XPT2046_CS 33
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25

// Touch Calibration Bounds (Adjustable if required for specific CYD batches)
#define TOUCH_MIN_X 300
#define TOUCH_MAX_X 3800
#define TOUCH_MIN_Y 250
#define TOUCH_MAX_Y 3850

// =============================================================================
// PN532 NFC Module (HSU - High Speed UART mode)
// =============================================================================
// Connected via CN1 / Extended header
// CYD GPIO 35 is Input-Only -> Ideal for UART RX
// CYD GPIO 22 is Output/Bidirectional -> Ideal for UART TX
#define PN532_UART_NUM 2
#define PN532_RX_PIN 27      // Connect to PN532 TXD
#define PN532_TX_PIN 22      // Connect to PN532 RXD
#define PN532_RESET_PIN 0xFF // No HW reset pin connected
#define PN532_BAUDRATE 115200

// =============================================================================
// Built-in CYD Peripherals
// =============================================================================
// RGB Status LED (Active LOW on ESP32-2432S028)
#define LED_RED_PIN 4
#define LED_GREEN_PIN 16
#define LED_BLUE_PIN 17
#define LED_ACTIVE_LEVEL LOW
#define LED_INACTIVE_LEVEL HIGH

// Audio / Piezo Buzzer
#define BUZZER_PIN 26
#define BUZZER_PWM_CHANNEL 0

// Backlight Pin
#define TFT_BL_PIN 21

// Light Sensor (LDR)
#define LDR_PIN 34
