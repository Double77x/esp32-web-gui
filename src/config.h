#pragma once
#include <Arduino.h> // Include for PROGMEM

// =========================================================================
// SPI & TOUCH PINS
// =========================================================================
#define TOUCH_SPI_MISO 39
#define TOUCH_SPI_MOSI 32
#define TOUCH_SPI_SCLK 25
#define TOUCH_CS 33
#define TOUCH_IRQ 36

// =========================================================================
// CALIBRATION
// =========================================================================
#define TOUCH_X_MIN 265
#define TOUCH_X_MAX 3843
#define TOUCH_Y_MIN 280
#define TOUCH_Y_MAX 3865

// =========================================================================
// WIFI & WEB
// =========================================================================
#define ALLOW_INSECURE_TEST 0

extern const char* PARAM_INPUT; // This is now unused, but we'll leave it

// =========================================================================
// SCREEN & UI
// =========================================================================
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2
#define HEADER_H 28
#define FOOTER_H 24
#define USE_FREE_FONTS 1

// =========================================================================
// WEB HTML & CERTIFICATES (Declarations ONLY)
// =========================================================================
extern const char index_html[] PROGMEM;
extern const char test_root_ca[] PROGMEM;
extern const char open_meteo_ca[] PROGMEM;