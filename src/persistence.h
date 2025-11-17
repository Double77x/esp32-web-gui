#pragma once

// Initializes LittleFS and loads all saved settings into their
// global variables. If settings files don't exist, it loads
// the defaults from secrets.h.
void loadConfig();

// Saves the current stockTickerList to stocks.json
void saveStockList();

// Saves the current weatherLocationList to weather.json
void saveWeatherList();

// Saves the current network (currentSsid, currentPass) to wifi.json
void saveWifiConfig();

// Saves the current app settings (e.g., rotationInterval) to settings.json
void saveAppSettings();