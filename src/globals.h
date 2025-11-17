#pragma once

// Include main library headers for type definitions
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <ESPAsyncWebServer.h>
#include <vector>        // <-- Added for lists
#include <Arduino.h>     // <-- Added for String

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
// Network State (NEW)
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
extern uint16_t CAT_BG;
extern uint16_t CAT_TEXT;
extern uint16_t CAT_MUTED;
extern uint16_t CAT_ACCENT;
extern uint16_t CAT_GREEN;
extern uint16_t CAT_RED;
extern uint16_t CAT_SURFACE;