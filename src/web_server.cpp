#include "web_server.h"
#include "globals.h"  // For server, inputUpdated, etc.
#include "config.h"   // For index_html
#include "secrets.h"  // Secrets
#include "utils.h"    // For to_upper
#include "drawing.h"  // For updateHeaderIP(), drawStatusMessage()
#include <vector>     // For list manipulation
#include <ArduinoJson.h> // For sending lists to web GUI
#include <algorithm>  // For std::find
#include <WiFi.h>     // For WiFi controls
#include <Update.h>   // For OTA Updates

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
      String ticker = request->getParam("ticker")->value();
      ticker.trim();
      ticker.replace(" ", "");
      to_upper(ticker.c_str(), upperString);
      
      lastTicker = upperString;
      inputUpdated = true; // This triggers the redraw in main.cpp
      
      Serial.println("Web Form Stock Input:");
      Serial.println(lastTicker);
    }
    request->redirect("/");
  });

  // --- One-off Weather Fetch ---
  server.on("/get_weather", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam("location")) {
      lastWeatherLocation = request->getParam("location")->value();
      weatherInputUpdated = true; // This triggers the redraw in main.cpp

      Serial.println("Web Form Weather Input:");
      Serial.println(lastWeatherLocation);
    }
    request->redirect("/");
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
        stockTickerList.push_back(newTicker);
      }
    }
    request->send(200, "text/plain", "OK"); // Send simple OK
  });
  server.on("/remove_stock", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ticker")) {
      String tickerToRemove = request->getParam("ticker")->value();
      // Find and remove the ticker
      auto it = std::find(stockTickerList.begin(), stockTickerList.end(), tickerToRemove);
      if (it != stockTickerList.end()) {
        stockTickerList.erase(it);
      }
    }
    request->send(200, "text/plain", "OK"); // Send simple OK
  });

  // --- API to Add/Remove Locations (No Redirect) ---
  server.on("/add_location", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("location")) {
      String newLoc = request->getParam("location")->value();
      newLoc.trim();
      if (newLoc.length() > 0) {
        weatherLocationList.push_back(newLoc);
      }
    }
    request->send(200, "text/plain", "OK"); // Send simple OK
  });
  server.on("/remove_location", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("location")) {
      String locToRemove = request->getParam("location")->value();
      auto it = std::find(weatherLocationList.begin(), weatherLocationList.end(), locToRemove);
      if (it != weatherLocationList.end()) {
        weatherLocationList.erase(it);
      }
    }
    request->send(200, "text/plain", "OK"); // Send simple OK
  });

  // --- API for Network Status ---
  server.on("/get_network_status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<256> doc;
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
      
      if (newSsid.length() > 0 && newPass.length() > 0) {
        Serial.printf("Received new WiFi credentials for: %s\n", newSsid.c_str());
        currentSsid = newSsid; // Update global
        currentPass = newPass; // Update global
        
        drawStatusMessage("Connecting...", CAT_ACCENT);
        WiFi.disconnect();
        WiFi.begin(currentSsid.c_str(), currentPass.c_str());
        
        // Give it 10 seconds to connect
        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < 20) {
          delay(500);
          Serial.print(".");
          retries++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nNew network connected!");
          updateHeaderIP(); // Redraw header with new IP
        } else {
          Serial.println("\nFailed to connect. Reverting to default.");
          drawStatusMessage("Connection Failed", CAT_RED);
          currentSsid = ssid; // Revert global
          currentPass = password; // Revert global
          WiFi.disconnect();
          WiFi.begin(currentSsid.c_str(), currentPass.c_str());
          // Wait for default to reconnect
          retries = 0;
          while (WiFi.status() != WL_CONNECTED && retries < 20) {
            delay(500);
            Serial.print(".");
            retries++;
          }
          updateHeaderIP();
        }
      }
    }
    request->redirect("/");
  });

  // --- OTA UPDATE HANDLERS ---
  
  // This handler receives the .bin file
  server.on("/update", HTTP_POST, 
    [](AsyncWebServerRequest *request){ // On success/failure
      // This is called AFTER the upload is finished
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", 
        Update.hasError() ? "<h1>OTA FAILED</h1><a href='/'>Retry?</a>" : "<h1>OTA OK!</h1><p>Rebooting...</p><script>setTimeout(()=>window.location='/',3000);</script>");
      response->addHeader("Connection", "close");
      request->send(response);
      
      if (!Update.hasError()) {
        delay(1000);
        ESP.restart(); // Reboot on success
      }
    }, 
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){ // On file upload
      // This is called FOR EACH CHUNK of the file
      if (index == 0) {
        Serial.printf("Update Start: %s\n", filename.c_str());
        drawStatusMessage("OTA Update...", CAT_ACCENT); // Show on TFT
        // Start OTA
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { 
          Update.printError(Serial);
        }
      }
      
      if (!Update.hasError()) {
        if (Update.write(data, len) != len) { // Write chunk
          Update.printError(Serial);
        }
      }
      
      if (final) {
        if (Update.end(true)) { // Finish OTA
          Serial.printf("Update Success: %u bytes\n", index + len);
        } else {
          Update.printError(Serial);
        }
      }
  });


  server.onNotFound(notFound);
  server.begin();
}