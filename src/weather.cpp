#include "weather.h"
#include "globals.h"    // For tft, colors, etc.
#include "config.h"     // For CAs
#include "drawing.h"    // For drawHeader, etc.
#include "utils.h"      // For HTTPSRequest
#include <ArduinoJson.h>
#include "Free_Fonts.h" // For FSSB12, FSSB18, etc.

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

  // --- Step 1: Geocoding API to get Lat/Lon from Name ---
  
  // Create a new, clean string for the URL
  String locNameFormatted = locationName;


  locNameFormatted.trim();
  locNameFormatted.replace(",", "");    
  locNameFormatted.replace(" ", "&");

  
  String geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name=";
  geoUrl += locNameFormatted; // Add the clean, formatted name
  geoUrl += "&count=1";

  // *** ADD THIS DEBUG LINE ***
  Serial.print("Geocoding URL: ");
  Serial.println(geoUrl);
  // **************************
  
  String geoResponse = HTTPSRequest(geoUrl, open_meteo_ca);
  
  StaticJsonDocument<1024> geoDoc;
  DeserializationError geoError = deserializeJson(geoDoc, geoResponse);

  // Check for valid response - *** IMPROVED ERROR CHECKING ***
  if (geoError) {
    // This block handles JSON parsing errors
    Serial.print("Geocoding deserialize failed: ");
    Serial.println(geoError.c_str());
    drawStatusMessage("Location not found", CAT_RED);
    return;
  }

  if (!geoDoc.containsKey("results") || geoDoc["results"].size() == 0) {
    // This block handles valid JSON that just has no results
    Serial.println("Geocoding failed: API returned no results.");
    drawStatusMessage("Location not found", CAT_RED);
    return;
  }
  // *** END OF IMPROVED CHECKING ***

  String lat = geoDoc["results"][0]["latitude"].as<String>();
  String lon = geoDoc["results"][0]["longitude"].as<String>();
  Serial.printf("Found Lat/Lon: %s, %s\n", lat.c_str(), lon.c_str());

  // --- Step 2: Forecast API to get Weather from Lat/Lon ---
  drawStatusMessage("Fetching weather...", CAT_MUTED);
  
  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += lat;
  url += "&longitude=";
  url += lon;
  url += "&current=temperature_2m,weather_code&temperature_unit=celsius";
  
  String response = HTTPSRequest(url, open_meteo_ca);

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, response);

  String temp = "N/A";
  String description = "Unknown";

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
  } else {
    if (doc.containsKey("current")) {
      temp = doc["current"]["temperature_2m"].as<String>();
      int code = doc["current"]["weather_code"].as<int>();
      description = getWeatherDescription(code);
    }
  }

  // --- Step 3: Draw the results ---
  tft.fillScreen(CAT_BG);
  drawHeader("Weather");
  drawFooter(PAGE_WEATHER);
  
  tft.setTextDatum(MC_DATUM); 

  // --- DRAW CITY (using the locationName) ---
  tft.setTextColor(CAT_MUTED, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  tft.drawString(locationName, SCREEN_WIDTH / 2, HEADER_H + 25);

  // --- DRAW WEATHER DESCRIPTION ---
  tft.setTextColor(CAT_TEXT, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB24);
  tft.setTextSize(1);
#else
  tft.setTextFont(8);
  tft.setTextSize(1);
#endif
  tft.drawString(description, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);

  // --- DRAW TEMPERATURE ---
  tft.setTextColor(CAT_ACCENT, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB18);
  tft.setTextSize(1);
#else
  tft.setTextFont(7);
  tft.setTextSize(1);
#endif
  tft.drawString(temp + "°C", SCREEN_WIDTH / 2, SCREEN_HEIGHT - FOOTER_H - 35);
}