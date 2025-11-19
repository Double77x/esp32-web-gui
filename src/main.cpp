// =========================================================================
// LIBRARY INCLUDES
// =========================================================================
#include <Arduino.h> // Required in main.cpp
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "Free_Fonts.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <ESPmDNS.h> // For .local address
#include <LittleFS.h>
#include <FS.h>

// =========================================================================
// MODULE INCLUDES
// =========================================================================
#include "globals.h"
#include "config.h"
#include "secrets.h" 
#include "utils.h"
#include "drawing.h"
#include "stocks.h"
#include "weather.h"
#include "web_server.h"
#include "persistence.h" // For persistence

// =========================================================================
// GLOBAL OBJECT DEFINITIONS (Matching externs in globals.h)
// =========================================================================
// Hardware Objects
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
AsyncWebServer server(80);

// Global State
Page currentPage = PAGE_STOCKS;
bool needsRedraw = true;
String lastTicker; // Will be set by loadConfig
String lastWeatherLocation; // Will be set by loadConfig
bool inputUpdated = false;
bool weatherInputUpdated = false;
char upperString[100];

// Network State
String currentSsid;
String currentPass;

// Rotation Lists & State
std::vector<String> stockTickerList;
std::vector<String> weatherLocationList;
int currentStockIndex = 0;
int currentLocIndex = 0;
unsigned long rotationInterval; // <-- THIS IS THE MISSING DEFINITION

// =========================================================================
// FILE-SCOPE STATIC VARIABLES
// =========================================================================
// Timer for auto-rotation
static unsigned long lastRotationTime = 0;

// =========================================================================
// FUNCTION PROTOTYPES
// =========================================================================
void checkTouch();

// =========================================================================
// SETUP
// =========================================================================
void setup() {
  Serial.begin(115200);

  // --- 1. Init Touch (Do this first) ---
  Serial.println("Initializing custom touch SPI bus (VSPI)...");
  SPI.begin(TOUCH_SPI_SCLK, TOUCH_SPI_MISO, TOUCH_SPI_MOSI);
  ts.begin();
  ts.setRotation(1); // Match TFT rotation
  Serial.println("Touchscreen initialized on separate SPI bus.");

  // --- 2. Init TFT ---
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true); 
  
  // --- 3. Init Colors & Screen ---
  init_colors(); // From drawing.cpp
  tft.fillScreen(CAT_BG);

  // --- 4. Load Config from Flash ---
  // This initializes LittleFS and loads all saved settings
  // (WiFi, Lists, Timer) into the global variables.
  loadConfig(); 

  // Set initial item to fetch
  if (!stockTickerList.empty()) {
    lastTicker = stockTickerList[0];
  }
  if (!weatherLocationList.empty()) {
    lastWeatherLocation = weatherLocationList[0];
  }

  // --- 5. Connect to WiFi (using loaded creds) ---
  // currentSsid and currentPass were set by loadConfig()
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname("esp32-web");
  WiFi.begin(currentSsid.c_str(), currentPass.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    // Even if WiFi fails, continue to start web server for OTA/config
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Start mDNS (Must be after WiFi connect) ---
  if (!MDNS.begin("esp32-ticker")) {
    Serial.println("Error setting up MDNS!");
  } else {
    Serial.println("mDNS responder started. Visit http://esp32-ticker.local");
    MDNS.addService("http", "tcp", 80);
  }

  // --- 6. Sync Time ---
  Serial.println("Setting up time sync...");
  drawHeader("Booting...");
  drawFooter(PAGE_STOCKS); 
  drawStatusMessage("Syncing Time...", CAT_MUTED);
  
  configTzTime("GMT0", "pool.ntp.org"); 
  
  time_t now = time(nullptr);
  int retries = 0;
  while (now < 1672531200L && retries < 30) {
      Serial.print(".");
      delay(500);
      now = time(nullptr);
      retries++;
  }
  if (now < 1672531200L) {
    Serial.println("\nTime sync FAILED! SSL requests will fail.");
    drawStatusMessage("Time Sync Failed", CAT_RED);
    delay(2000); 
  } else {
    Serial.println("\nTime synced!");
  }
  
  // --- 7. Start Web Server ---
  setup_web_server(); // From web_server.cpp
}

// =========================================================================
// MAIN LOOP (The State Machine)
// =========================================================================
void loop() {
  if (lastRotationTime == 0) { // First-run initialization
      lastRotationTime = millis();
  }

  // 1. Check for user touch input
  checkTouch();

  // 2. Check for one-off web stock fetch
  if (inputUpdated) {
    inputUpdated = false;
    currentPage = PAGE_STOCKS;
    needsRedraw = true;
    lastRotationTime = millis(); // Reset rotation timer
  }

  // 3. Check for one-off web weather fetch
  if (weatherInputUpdated) {
    weatherInputUpdated = false;
    currentPage = PAGE_WEATHER;
    needsRedraw = true;
    lastRotationTime = millis(); // Reset rotation timer
  }

  // 4. Check for auto-rotation
  if (millis() - lastRotationTime > rotationInterval) {
    lastRotationTime = millis();
    
    // Toggle the page
    currentPage = (currentPage == PAGE_STOCKS) ? PAGE_WEATHER : PAGE_STOCKS;
    
    if (currentPage == PAGE_STOCKS) {
      // Advance to next stock in list
      if (!stockTickerList.empty()) {
        currentStockIndex = (currentStockIndex + 1) % stockTickerList.size();
        lastTicker = stockTickerList[currentStockIndex];
      }
    } else {
      // Advance to next location in list
      if (!weatherLocationList.empty()) {
        currentLocIndex = (currentLocIndex + 1) % weatherLocationList.size();
        lastWeatherLocation = weatherLocationList[currentLocIndex];
      }
    }
    needsRedraw = true;
  }

  // 5. Redraw the screen if needed
  if (needsRedraw) {
    needsRedraw = false;
    
    if (currentPage == PAGE_STOCKS) {
      fetchAndDisplayTicker(lastTicker);
    } else {
      fetchAndDisplayWeather(lastWeatherLocation);
    }
  }
}

// =========================================================================
// LOCAL-ONLY FUNCTIONS
// =========================================================================
void checkTouch() {
  static uint32_t lastTouchTime = 0;

  if (millis() - lastTouchTime < 500) {
    return;
  }
  
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    if (p.z > 100 && p.z < 3000) {
      lastTouchTime = millis(); // Used to debounce touch
      
      // Convert raw ADC values to screen pixels
      int16_t x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, 320);
      int16_t y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, 240);
      
      Serial.print("Touch detected! Toggling page. ");
      Serial.printf("[Raw: x=%d, y=%d] [Mapped: x=%d, y=%d]\n", p.x, p.y, x, y);
      
      currentPage = (currentPage == PAGE_STOCKS) ? PAGE_WEATHER : PAGE_STOCKS;
      needsRedraw = true;
      lastRotationTime = millis(); // Also reset auto-rotation timer
    }
  }
}