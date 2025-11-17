#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "persistence.h"
#include "globals.h"
#include "secrets.h"
#include "drawing.h"

// Helper function to read a file
String readFile(const char* path) {
  fs::File file = LittleFS.open(path, "r"); // <-- FIX: Use fs::File
  if (!file) {
    Serial.printf("Failed to open file %s for reading\n", path);
    return "";
  }
  String data = file.readString();
  file.close();
  return data;
}

// Helper function to write a file
void writeFile(const char* path, const String& data) {
  fs::File file = LittleFS.open(path, "w"); // <-- FIX: Use fs::File
  if (!file) {
    Serial.printf("Failed to open file %s for writing\n", path);
    return;
  }
  if (file.print(data)) {
    Serial.printf("Successfully wrote to file %s\n", path);
  } else {
    Serial.printf("Failed to write to file %s\n", path);
  }
  file.close();
}

// Helper function to save a std::vector<String> to a file
void saveList(const char* path, std::vector<String>& list) {
  StaticJsonDocument<1024> doc;
  JsonArray array = doc.to<JsonArray>();
  for (const String& item : list) {
    array.add(item);
  }
  String json;
  serializeJson(doc, json);
  writeFile(path, json);
}

// --- Public Functions ---

void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    drawStatusMessage("Storage Error", CAT_RED);
    // Load defaults as a fallback
    rotationInterval = 60000;
    currentSsid = ssid;
    currentPass = password;
    stockTickerList = defaultStockList;
    weatherLocationList = defaultWeatherList;
    return;
  }
  Serial.println("LittleFS mounted.");

  // 1. Load Settings (Rotation Timer)
  String settingsData = readFile("/settings.json");
  StaticJsonDocument<256> settingsDoc;
  if (settingsData.length() > 0 && deserializeJson(settingsDoc, settingsData) == DeserializationError::Ok) {
    
    // Check if the key exists. If not, use the default.
    if (settingsDoc.containsKey("rotation_ms")) {
      rotationInterval = settingsDoc["rotation_ms"].as<unsigned long>();
    } else {
      rotationInterval = 60000; // Default 60s
    }

    Serial.printf("Loaded interval: %lu ms\n", rotationInterval);
  } else {
    Serial.println("No settings.json found, loading default interval.");
    rotationInterval = 60000; // 1 minute
  }

  // 2. Load WiFi Config
  String wifiData = readFile("/wifi.json");
  StaticJsonDocument<256> wifiDoc;
  if (wifiData.length() > 0 && deserializeJson(wifiDoc, wifiData) == DeserializationError::Ok) {
    currentSsid = wifiDoc["ssid"].as<String>();
    currentPass = wifiDoc["pass"].as<String>();
    Serial.printf("Loaded WiFi config for: %s\n", currentSsid.c_str());
  } else {
    Serial.println("No wifi.json found, loading default WiFi.");
    currentSsid = ssid; // from secrets.h
    currentPass = password; // from secrets.h
  }

  // 3. Load Stock List
  String stocksData = readFile("/stocks.json");
  StaticJsonDocument<1024> stocksDoc;
  if (stocksData.length() > 0 && deserializeJson(stocksDoc, stocksData) == DeserializationError::Ok) {
    stockTickerList.clear();
    for (JsonVariant item : stocksDoc.as<JsonArray>()) {
      stockTickerList.push_back(item.as<String>());
    }
    Serial.println("Loaded saved stock list.");
  } else {
    Serial.println("No stocks.json found, loading default stock list.");
    stockTickerList = defaultStockList; // from secrets.h
  }

  // 4. Load Weather List
  String weatherData = readFile("/weather.json");
  StaticJsonDocument<1024> weatherDoc;
  if (weatherData.length() > 0 && deserializeJson(weatherDoc, weatherData) == DeserializationError::Ok) {
    weatherLocationList.clear();
    for (JsonVariant item : weatherDoc.as<JsonArray>()) {
      weatherLocationList.push_back(item.as<String>());
    }
    Serial.println("Loaded saved weather list.");
  } else {
    Serial.println("No weather.json found, loading default weather list.");
    weatherLocationList = defaultWeatherList; // from secrets.h
  }
}

void saveStockList() {
  Serial.println("Saving stock list to flash...");
  saveList("/stocks.json", stockTickerList);
}

void saveWeatherList() {
  Serial.println("Saving weather list to flash...");
  saveList("/weather.json", weatherLocationList);
}

void saveWifiConfig() {
  Serial.println("Saving WiFi config to flash...");
  StaticJsonDocument<256> doc;
  doc["ssid"] = currentSsid;
  doc["pass"] = currentPass;
  String json;
  serializeJson(doc, json);
  writeFile("/wifi.json", json);
}

void saveAppSettings() {
  Serial.println("Saving app settings to flash...");
  StaticJsonDocument<256> doc;
  doc["rotation_ms"] = rotationInterval;
  String json;
  serializeJson(doc, json);
  writeFile("/settings.json", json);
}