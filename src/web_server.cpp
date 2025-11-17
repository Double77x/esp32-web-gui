#include "web_server.h"
#include "globals.h"  // For server, inputUpdated, etc.
#include "config.h"   // For index_html
#include "secrets.h"  // For default ssid/password
#include "utils.h"    // For to_upper
#include "drawing.h"  // For updateHeaderIP(), drawStatusMessage()
#include "persistence.h" // For saving settings
#include <vector>
#include <ArduinoJson.h>
#include <algorithm> // For std::find
#include <WiFi.h>
#include <Update.h>  // For OTA Updates

// Handles 404 Not Found
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Main setup function for all server endpoints
void setup_web_server() {
  
  // --- Main Web Page ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  // --- One-off Stock Fetch ---
  server.on("/get_stock", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("ticker")) {
      lastTicker = request->getParam("ticker")->value();
      lastTicker.trim();
      lastTicker.toUpperCase();
      inputUpdated = true; // Flag for main loop
    }
    request->redirect("/"); // Redirect back to main page
  });

  // --- One-off Weather Fetch ---
  server.on("/get_weather", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("location")) {
      lastWeatherLocation = request->getParam("location")->value();
      lastWeatherLocation.trim();
      weatherInputUpdated = true; // Flag for main loop
    }
    request->redirect("/"); // Redirect back to main page
  });

  // --- API to load lists on web page ---
  server.on("/get_lists", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<1024> doc;
    JsonArray stocks = doc.createNestedArray("stocks");
    for (const String& ticker : stockTickerList) {
      stocks.add(ticker);
    }
    JsonArray locations = doc.createNestedArray("locations");
    for (const String& loc : weatherLocationList) {
      locations.add(loc);
    }
    // Add the current rotation interval
    doc["interval_sec"] = rotationInterval / 1000; // Send as seconds
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  // --- API to Add/Remove Stocks (No Redirect) ---
  server.on("/add_stock", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ticker")) {
      String newTicker = request->getParam("ticker")->value();
      newTicker.trim();
      newTicker.toUpperCase();
      if (newTicker.length() > 0) {
        // Prevent duplicates
        if (std::find(stockTickerList.begin(), stockTickerList.end(), newTicker) == stockTickerList.end()) {
          stockTickerList.push_back(newTicker);
          saveStockList(); // Save to flash
        }
      }
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/remove_stock", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ticker")) {
      String tickerToRemove = request->getParam("ticker")->value();
      auto it = std::find(stockTickerList.begin(), stockTickerList.end(), tickerToRemove);
      if (it != stockTickerList.end()) {
        stockTickerList.erase(it);
        saveStockList(); // Save to flash
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // --- API to Add/Remove Locations (No Redirect) ---
  server.on("/add_location", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("location")) {
      String newLoc = request->getParam("location")->value();
      newLoc.trim();
      if (newLoc.length() > 0) {
        // Prevent duplicates
        if (std::find(weatherLocationList.begin(), weatherLocationList.end(), newLoc) == weatherLocationList.end()) {
          weatherLocationList.push_back(newLoc);
          saveWeatherList(); // Save to flash
        }
      }
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/remove_location", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("location")) {
      String locToRemove = request->getParam("location")->value();
      auto it = std::find(weatherLocationList.begin(), weatherLocationList.end(), locToRemove);
      if (it != weatherLocationList.end()) {
        weatherLocationList.erase(it);
        saveWeatherList(); // Save to flash
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // --- NEW ENDPOINT: Update List Order ---
  server.on("/update_lists", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("type") && request->hasParam("list")) {
      String type = request->getParam("type")->value();
      String listStr = request->getParam("list")->value();

      std::vector<String> newList;
      // Basic string split logic
      String currentItem = "";
      for (int i = 0; i < listStr.length(); i++) {
        if (listStr[i] == ',') {
          if (currentItem.length() > 0) {
            newList.push_back(currentItem);
          }
          currentItem = "";
        } else {
          currentItem += listStr[i];
        }
      }
      // Add the last item
      if (currentItem.length() > 0) {
        newList.push_back(currentItem);
      }

      // Overwrite the global list and save
      if (type == "stocks") {
        stockTickerList = newList;
        saveStockList();
        Serial.println("Updated stock list order.");
      } else if (type == "locations") {
        weatherLocationList = newList;
        saveWeatherList();
        Serial.println("Updated weather list order.");
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // --- API: Set Rotation Interval ---
  server.on("/set_interval", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("interval_sec")) {
      long newInterval = request->getParam("interval_sec")->value().toInt();
      if (newInterval >= 10) { // Enforce a minimum
        rotationInterval = newInterval * 1000; // Convert sec to ms
        saveAppSettings(); // Save to settings.json
        Serial.printf("Rotation interval set to: %lu ms\n", rotationInterval);
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // --- NEW ENDPOINT: Restore Default Lists ---
  server.on("/restore_defaults", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Restoring default lists from secrets...");
    
    // Copy from secrets (defined in secrets.cpp, extern in secrets.h)
    stockTickerList = defaultStockList;
    weatherLocationList = defaultWeatherList;
    
    // Reset indices
    currentStockIndex = 0;
    currentLocIndex = 0;

    // Save these defaults back to the persistence files
    saveStockList();
    saveWeatherList();
    
    request->send(200, "text/plain", "OK");
  });

  // --- API for Network Status ---
  server.on("/get_network_status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<128> doc;
    doc["ssid"] = currentSsid;
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  // --- API for Network Connect ---
  server.on("/connect_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid") && request->hasParam("pass")) {
      String newSsid = request->getParam("ssid")->value();
      String newPass = request->getParam("pass")->value();

      if (newSsid.length() > 0) {
        Serial.printf("Connecting to new network: %s\n", newSsid.c_str());
        drawStatusMessage("Connecting...", CAT_ACCENT);
        
        currentSsid = newSsid; // Update global
        currentPass = newPass; // Update global
        
        WiFi.disconnect();
        WiFi.begin(currentSsid.c_str(), currentPass.c_str());
        
        // Wait 10 seconds for connection
        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
          delay(500);
          Serial.print(".");
          retries++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nNew network connected!");
          updateHeaderIP(); // Redraw header with new IP
          saveWifiConfig(); // Save the new working config
        } else {
          Serial.println("\nFailed to connect. Reverting to default.");
          drawStatusMessage("Connection Failed", CAT_RED);
          // Revert to default logic (from secrets.h)
          currentSsid = ssid;
          currentPass = password;
          WiFi.disconnect();
          WiFi.begin(currentSsid.c_str(), currentPass.c_str());
          saveWifiConfig(); // Re-save the default
          // Wait for default to connect
          while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print("r");
          }
          updateHeaderIP();
        }
      }
    }
    request->redirect("/");
  });

  // --- OTA UPDATE HANDLERS ---
  // Page to show on successful update
  server.on("/ota_success", HTTP_GET, [](AsyncWebServerRequest *request){
    String successHtml = "<html><head><title>OTA Success</title><meta charset='utf-8' name='viewport' content='width=device-width, initial-scale=1'>"
                         "<style>body{background:#1E1E2E;color:#DCE0E8;font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}"
                         ".card{background:#11111b;padding:20px;border-radius:12px;text-align:center;}h1{color:#ABE9B3;}a{color:#89B4FA;}</style></head>"
                         "<body><div class='card'><h1>OTA Update Successful!</h1><p>Device is rebooting...</p><p><a href='/'>Click to return to home</a></p></div></body></html>";
    request->send(200, "text/html", successHtml);
  });
  
  // Handler for the firmware file upload
  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // This is called when the update is complete
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots...");
      response->addHeader("Location", "/ota_success");
      response->addHeader("Connection", "close");
      request->send(response);
      
      ESP.restart(); // Restart the ESP
      
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // This is called for each chunk of the file
      if (index == 0) {
        Serial.printf("OTA Update Start: %s\n", filename.c_str());
        drawStatusMessage("OTA Update...", CAT_ACCENT);
        
        // Start the update process
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Use max available space
          Update.printError(Serial);
        }
      }

      // Write the chunk to the flash
      if (len) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }

      // If this is the last chunk
      if (final) {
        if (Update.end(true)) { // true to commit the update
          Serial.printf("Update Success: %u bytes\n", index + len);
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  server.onNotFound(notFound);
  server.begin();
}