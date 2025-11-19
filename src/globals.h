#pragma once

// Include main library headers for type definitions
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ESPAsyncWebServer.h>
#include <vector>        // For lists
#include <Arduino.h>     // For String

// ==========
// Enum Definitions
// ==========
enum Page { PAGE_STOCKS, PAGE_WEATHER };

// ==========
// Hardware Objects
// ==========
extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;
extern AsyncWebServer server;

// ==========
// Global State
// ==========
extern Page currentPage;
extern bool needsRedraw;
extern String lastTicker; // For one-off fetch
extern String lastWeatherLocation; // For one-off fetch

extern bool inputUpdated; // One-off stock fetch
extern bool weatherInputUpdated; // One-off weather fetch
extern char upperString[100];

// ==========
// Network State
// ==========
extern String currentSsid;
extern String currentPass;

// ==========
// Rotation Lists & State
// ==========
extern std::vector<String> stockTickerList;
extern std::vector<String> weatherLocationList;
extern int currentStockIndex;
extern int currentLocIndex;
extern unsigned long rotationInterval;

// ==========
// Global Colors
// ==========
// UI Basics
extern uint16_t CAT_BG;      // Background
extern uint16_t CAT_TEXT;    // Main Text
extern uint16_t CAT_MUTED;   // Secondary/Dim Text
extern uint16_t CAT_ACCENT;  // Highlights
extern uint16_t CAT_SURFACE; // Card backgrounds/Headers

// Indicators
extern uint16_t CAT_GREEN;   // Positive/Go
extern uint16_t CAT_RED;     // Negative/Stop
extern uint16_t CAT_YELLOW;  // Sun/Lightning/Warnings
extern uint16_t CAT_BLUE;    // Rain/Water
extern uint16_t CAT_WHITE;   // Pure White (High Contrast)
extern uint16_t CAT_GREY;    // Neutral/Clouds