#pragma once
#include <Arduino.h>
#include <vector>

// This file DECLARES your secret variables.
// The actual values are in secrets.cpp (which is ignored by Git).

// --- WiFi Credentials ---
extern const char* ssid;
extern const char* password;

// --- API Keys ---
extern const char* finnhub_api_key;

// --- Default Rotation Lists ---
extern std::vector<String> defaultStockList;
extern std::vector<String> defaultWeatherList;