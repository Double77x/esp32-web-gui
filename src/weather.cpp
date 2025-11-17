#include "weather.h"
#include "globals.h"    // For tft, colors, etc.
#include "config.h"     // For CAs
#include "drawing.h"    // For drawHeader, etc.
#include "utils.h"      // For HTTPSRequest
#include <ArduinoJson.h>
#include "Free_Fonts.h" // For FSSB12, FSSB18, etc.
#include <time.h>       // For gmtime() to get day of week

// --- NEW HELPER FUNCTION ---
// Converts a unixtime stamp to a 3-letter day of the week
String getDayOfWeek(time_t unixtime) {
  // Use gmtime to get the day of the week (0=Sun, 1=Mon, ...)
  struct tm * timeinfo;
  timeinfo = gmtime(&unixtime);
  
  switch(timeinfo->tm_wday) {
    case 0: return "SUN";
    case 1: return "MON";
    case 2: return "TUE";
    case 3: return "WED";
    case 4: return "THU";
    case 5: return "FRI";
    case 6: return "SAT";
    default: return "???";
  }
}
// --- END NEW FUNCTION ---

// Updated to return text descriptions
String getWeatherDescription(int code) {
  if (code == 0) return "Sunny";
  if (code == 1) return "Mostly Sunny";
  if (code == 2) return "Cloudy";
  if (code == 3) return "Overcast";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code == 80 || code == 81 || code == 82) return "Rain Shower";
  if (code == 85 || code == 86) return "Snow Shower";
  if (code == 95 || code == 96 || code == 99) return "Thunderstorm";
  return "Unknown";
}

// This function now takes a location NAME, not coordinates
void fetchAndDisplayWeather(String locationName) {
  Serial.printf("Fetching weather for: %s\n", locationName.c_str());
  
  drawHeader("Weather");
  drawFooter(PAGE_WEATHER);
  drawStatusMessage("Finding location...", CAT_MUTED);

  // --- Define lat/lon in the outer scope ---
  String lat, lon;

  // --- Step 1: Geocoding API (in its own scope to free memory) ---
  { // <--- START OF NEW SCOPE
    
    // Create a new, clean string for the URL
    String locNameFormatted = locationName;
    locNameFormatted.trim();
    // Replace space after comma with '&'
    if (locNameFormatted.indexOf(',') != -1) {
      locNameFormatted.replace(", ", "&");
    }
    // Any remaining spaces become '+'
    locNameFormatted.replace(" ", "+");
    
    // Use the correct Geocoding API domain
    String geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name=";
    geoUrl += locNameFormatted; // Add the clean, formatted name
    geoUrl += "&count=1";

    Serial.print("Geocoding URL: ");
    Serial.println(geoUrl);
    
    String geoResponse = HTTPSRequest(geoUrl, open_meteo_ca);
    
    DynamicJsonDocument geoDoc(1024); // Use DynamicJsonDocument for Heap
    DeserializationError geoError = deserializeJson(geoDoc, geoResponse);

    if (geoError) {
      Serial.print("Geocoding deserialize failed: ");
      Serial.println(geoError.c_str());
      drawStatusMessage("Location Error: " + String(geoError.c_str()), CAT_RED);
      return;
    }

    // Improved error checking
    if (!geoDoc.containsKey("results") || geoDoc["results"].size() == 0) {
      Serial.println("Geocoding failed: API returned no results.");
      drawStatusMessage("Location Error: No results", CAT_RED);
      return;
    }

    // Save lat/lon to the outer scope variables
    lat = geoDoc["results"][0]["latitude"].as<String>();
    lon = geoDoc["results"][0]["longitude"].as<String>();
    Serial.printf("Found Lat/Lon: %s, %s\n", lat.c_str(), lon.c_str());
    
  } // <--- END OF NEW SCOPE. geoDoc and geoResponse are now destroyed.

  // --- Step 2: Forecast API to get Weather from Lat/Lon ---
  
  drawStatusMessage("Fetching weather...", CAT_MUTED);
  
  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += lat;
  url += "&longitude=";
  url += lon;
  // --- FINAL, CORRECTED API CALL ---
  url += "&current=temperature_2m,weather_code";
  url += "&daily=weather_code,temperature_2m_max,temperature_2m_min"; // Removed ",time"
  url += "&temperature_unit=celsius&timeformat=unixtime&forecast_days=4";
  // --- END ---
  
  String response = HTTPSRequest(url, open_meteo_ca);

  // --- DEBUG: Print raw JSON response ---
  Serial.println("--- Open-Meteo Forecast Response ---");
  Serial.println(response);
  Serial.println("--------------------------------------");

  DynamicJsonDocument doc(4096); // Use DynamicJsonDocument for Heap
  DeserializationError error = deserializeJson(doc, response);

  // --- Start Drawing ---
  tft.fillScreen(CAT_BG);
  drawHeader("Weather");
  drawFooter(PAGE_WEATHER);
  tft.setTextDatum(MC_DATUM); 

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    drawStatusMessage("Forecast Error: " + String(error.c_str()), CAT_RED);
    return;
  } 
  
  // --- Check for valid data ---
  if (!doc.containsKey("current") || !doc.containsKey("daily")) {
    Serial.println("API response missing current or daily data.");
    drawStatusMessage("Forecast Error: Bad data", CAT_RED);
    return;
  }

  // --- Step 3: Draw Current Weather (Top Half) ---
  String tempToday = doc["current"]["temperature_2m"].as<String>();
  int codeToday = doc["current"]["weather_code"].as<int>();
  String descToday = getWeatherDescription(codeToday);
  
  // Draw City Name
  tft.setTextColor(CAT_MUTED, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  tft.drawString(locationName, SCREEN_WIDTH / 2, HEADER_H + 25);

  // Draw Current Temperature
  tft.setTextColor(CAT_TEXT, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB24); // Large font for current temp
  tft.setTextSize(1);
#else
  tft.setTextFont(8);
  tft.setTextSize(1);
#endif
  tft.drawString(tempToday + "°C", SCREEN_WIDTH / 2, 90); // Y=90

  // Draw Current Description
  tft.setTextColor(CAT_ACCENT, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB12); // Medium font
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  tft.drawString(descToday, SCREEN_WIDTH / 2, 125); // Y=125

  // --- Step 4: Draw 3-Day Forecast (Bottom Half) ---
  
  // Draw separator line
  tft.drawFastHLine(20, 150, SCREEN_WIDTH - 40, CAT_MUTED); // Y=150

  JsonArray dailyTime = doc["daily"]["time"];
  JsonArray dailyCode = doc["daily"]["weather_code"];
  JsonArray dailyMax = doc["daily"]["temperature_2m_max"];
  JsonArray dailyMin = doc["daily"]["temperature_2m_min"];

  // Define column X positions (centered)
  int x1 = 53;  // (320 / 3) / 2
  int x2 = 160; // (320 / 2)
  int x3 = 267; // 320 - (320 / 3) / 2
  
  int y_day = 168;
  int y_desc = 190;
  int y_temp = 210;
  
  // Loop from 1 to 3 (index 0 is today, 1 is tomorrow, etc.)
  for (int i = 1; i <= 3; i++) {
    if (dailyTime.size() <= i) break; // Safety check

    int x_pos = (i == 1) ? x1 : (i == 2) ? x2 : x3;
    
    // Get Day Name
    time_t time = dailyTime[i];
    String day = getDayOfWeek(time);
    
    // Get Description
    int code = dailyCode[i];
    String desc = getWeatherDescription(code);
    
    // Get Temps (as integers)
    String maxTemp = String(dailyMax[i].as<float>(), 0);
    String minTemp = String(dailyMin[i].as<float>(), 0);
    String tempRange = maxTemp + "°/" + minTemp + "°";
    
    // Draw Day
    tft.setTextColor(CAT_TEXT, CAT_BG);
#if USE_FREE_FONTS
    tft.setFreeFont(FSSB9); // Bold 9pt
    tft.setTextSize(1);
#else
    tft.setTextFont(2);
    tft.setTextSize(1);
#endif
    tft.drawString(day, x_pos, y_day);
    
    // Draw Description
    tft.setTextColor(CAT_MUTED, CAT_BG);
#if USE_FREE_FONTS
    tft.setFreeFont(FSS9); // Regular 9pt
    tft.setTextSize(1);
#else
    tft.setTextFont(2);
    tft.setTextSize(1);
#endif
    tft.drawString(desc, x_pos, y_desc);
    
    // Draw Temps
    tft.setTextColor(CAT_ACCENT, CAT_BG);
#if USE_FREE_FONTS
    tft.setFreeFont(FSSB9); // Bold 9pt
    tft.setTextSize(1);
#else
    tft.setTextFont(2);
    tft.setTextSize(1);
#endif
    tft.drawString(tempRange, x_pos, y_temp);
  }
}