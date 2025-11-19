#include "weather.h"
#include "globals.h"    // For tft, colors, etc.
#include "config.h"     // For CAs
#include "drawing.h"    // For drawHeader, etc.
#include "utils.h"      // For HTTPSRequest
#include <ArduinoJson.h>
#include "Free_Fonts.h" // For FSSB12, FSSB18, etc.
#include <time.h>       // For gmtime()

// --- HELPER: Convert WMO code to Text ---
String getWeatherDescription(int code) {
  if (code == 0) return "Sunny";
  if (code == 1) return "Mostly Sunny";
  if (code == 2) return "Cloudy";
  if (code == 3) return "Overcast";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 85 && code <= 86) return "Snow Showers";
  if (code >= 95 && code <= 99) return "Storms";
  return "Unknown";
}

// --- HELPER: Get Day Name ---
String getDayOfWeek(time_t unixtime) {
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

// --- HELPER: Draw Geometric Weather Icons ---
void drawWeatherIcon(int x, int y, int code, int size, bool isNight) {
  int r = size / 2; 
  
  // 0: Clear Sky (Sun)
  if (code == 0 || code == 1) {
    tft.fillCircle(x, y, r, CAT_YELLOW);
    if (code == 0) tft.drawCircle(x, y, r + 4, CAT_YELLOW);
  }
  
  // 2, 3, 45, 48: Cloudy / Fog
  else if (code == 2 || code == 3 || code == 45 || code == 48) {
    tft.fillCircle(x - (r/2), y + (r/4), r * 0.8, CAT_GREY); // Left puff
    tft.fillCircle(x + (r/2), y + (r/4), r * 0.8, CAT_GREY); // Right puff
    tft.fillCircle(x, y - (r/4), r, (code == 2) ? CAT_WHITE : CAT_GREY); // Main puff
  }
  
  // 51-67, 80-82: Rain
  else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
    // Cloud Base
    tft.fillCircle(x - (r/2), y, r * 0.7, CAT_GREY); 
    tft.fillCircle(x + (r/2), y, r * 0.7, CAT_GREY);
    tft.fillCircle(x, y - (r/4), r * 0.8, CAT_WHITE);
    // Rain Drops
    tft.drawLine(x - 5, y + r, x - 5, y + r + (r/2), CAT_BLUE);
    tft.drawLine(x + 5, y + r, x + 5, y + r + (r/2), CAT_BLUE);
    tft.drawLine(x, y + r + 5, x, y + r + (r/2) + 5, CAT_BLUE);
  }
  
  // 71-77, 85-86: Snow
  else if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) {
    // Cloud Base
    tft.fillCircle(x - (r/2), y, r * 0.7, CAT_GREY); 
    tft.fillCircle(x + (r/2), y, r * 0.7, CAT_GREY);
    tft.fillCircle(x, y - (r/4), r * 0.8, CAT_WHITE);
    // Snowflakes (White dots)
    tft.fillCircle(x - 5, y + r, 2, CAT_WHITE);
    tft.fillCircle(x + 5, y + r, 2, CAT_WHITE);
    tft.fillCircle(x, y + r + 5, 2, CAT_WHITE);
  }
  
  // 95-99: Thunderstorm
  else if (code >= 95 && code <= 99) {
    // Dark Cloud
    tft.fillCircle(x - (r/2), y, r * 0.7, 0x528A); 
    tft.fillCircle(x + (r/2), y, r * 0.7, 0x528A);
    tft.fillCircle(x, y - (r/4), r * 0.8, 0x7BEF);
    // Lightning Bolt (Yellow ZigZag)
    tft.drawLine(x, y + (r/2), x - 5, y + r, CAT_YELLOW);
    tft.drawLine(x - 5, y + r, x + 5, y + r, CAT_YELLOW);
    tft.drawLine(x + 5, y + r, x, y + r + (r/2) + 5, CAT_YELLOW);
  }
  else {
     // Unknown
     tft.setTextColor(CAT_TEXT);
     tft.drawString("?", x, y);
  }
}

// --- MAIN FUNCTION ---
void fetchAndDisplayWeather(String locationName) {
  Serial.printf("Fetching weather for: %s\n", locationName.c_str());
  
  drawHeader("Weather");
  drawFooter(PAGE_WEATHER);
  drawStatusMessage("Finding location...", CAT_MUTED);

  String lat, lon;

  // --- Step 1: Geocoding ---
  { 
    String locNameFormatted = locationName;
    locNameFormatted.trim();
    if (locNameFormatted.indexOf(',') != -1) locNameFormatted.replace(", ", "&");
    locNameFormatted.replace(" ", "+");
    
    String geoUrl = "https://geocoding-api.open-meteo.com/v1/search?name=" + locNameFormatted + "&count=1";
    String geoResponse = HTTPSRequest(geoUrl, open_meteo_ca);
    
    DynamicJsonDocument geoDoc(1024);
    DeserializationError geoError = deserializeJson(geoDoc, geoResponse);

    if (geoError || !geoDoc.containsKey("results") || geoDoc["results"].size() == 0) {
      drawStatusMessage("Loc Error", CAT_RED);
      return;
    }
    lat = geoDoc["results"][0]["latitude"].as<String>();
    lon = geoDoc["results"][0]["longitude"].as<String>();
  } 

  // --- Step 2: Forecast API ---
  drawStatusMessage("Fetching data...", CAT_MUTED);
  
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + lat + "&longitude=" + lon;
  url += "&current=temperature_2m,weather_code,is_day"; 
  url += "&daily=weather_code,temperature_2m_max,temperature_2m_min";
  url += "&temperature_unit=celsius&timeformat=unixtime&forecast_days=4";
  
  String response = HTTPSRequest(url, open_meteo_ca);
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, response);

  tft.fillScreen(CAT_BG);
  drawHeader("Weather");
  drawFooter(PAGE_WEATHER);
  tft.setTextDatum(MC_DATUM); 

  if (error || !doc.containsKey("current") || !doc.containsKey("daily")) {
    drawStatusMessage("API Error", CAT_RED);
    return;
  }

  // --- Step 3: Draw MAIN Current Weather ---
  String tempToday = String(doc["current"]["temperature_2m"].as<int>()); 
  int codeToday = doc["current"]["weather_code"].as<int>();
  String descToday = getWeatherDescription(codeToday);
  
  String maxToday = String(doc["daily"]["temperature_2m_max"][0].as<int>());
  String minToday = String(doc["daily"]["temperature_2m_min"][0].as<int>());

  // Location Name
  tft.setTextColor(CAT_MUTED, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  #else
  tft.setTextFont(4);
  #endif
  tft.drawString(locationName, SCREEN_WIDTH / 2, HEADER_H + 15);

  // Large Weather Icon 
  drawWeatherIcon(80, 95, codeToday, 50); 

  // Big Temp 
  tft.setTextColor(CAT_TEXT, CAT_BG);
  tft.setTextDatum(ML_DATUM); 
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB24); 
  #else
  tft.setTextFont(8);
  #endif
  tft.drawString(tempToday + "C", 140, 85);
  
  // Manual degree circle
  int tempWidth = tft.textWidth(tempToday);
  tft.drawCircle(140 + tempWidth + 6, 70, 3, CAT_TEXT); 

  // Description & High/Low 
  tft.setTextDatum(MC_DATUM); 
  tft.setTextColor(CAT_ACCENT, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB9);
  #else
  tft.setTextFont(2);
  #endif
  String subText = descToday + " (" + maxToday + "/" + minToday + ")";
  tft.drawString(subText, SCREEN_WIDTH / 2, 135);

  // --- Step 4: Draw 3-Day Forecast ---
  tft.drawFastHLine(10, 150, SCREEN_WIDTH - 20, CAT_MUTED);

  JsonArray dailyTime = doc["daily"]["time"];
  JsonArray dailyCode = doc["daily"]["weather_code"];
  JsonArray dailyMax = doc["daily"]["temperature_2m_max"];
  JsonArray dailyMin = doc["daily"]["temperature_2m_min"];

  int cardWidth = (SCREEN_WIDTH - 20) / 3; 
  int startY = 160;
  
  for (int i = 1; i <= 3; i++) {
    if (dailyTime.size() <= i) break;

    int centerX = 10 + (cardWidth * (i - 1)) + (cardWidth / 2);
    
    time_t time = dailyTime[i];
    String day = getDayOfWeek(time);
    int code = dailyCode[i];
    int maxT = dailyMax[i].as<int>();
    int minT = dailyMin[i].as<int>();
    
    // 1. Day Name
    tft.setTextColor(CAT_TEXT, CAT_BG);
    #if USE_FREE_FONTS
    tft.setFreeFont(FSSB9);
    #else
    tft.setTextFont(2);
    #endif
    tft.setTextDatum(MC_DATUM);
    tft.drawString(day, centerX, startY);

    // 2. Icon
    drawWeatherIcon(centerX, startY + 28, code, 30); 

    // 3. High / Low
    tft.setTextColor(CAT_MUTED, CAT_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("/", centerX, startY + 58);

    tft.setTextColor(CAT_WHITE, CAT_BG); 
    tft.setTextDatum(MR_DATUM); 
    tft.drawString(String(maxT), centerX - 5, startY + 58);

    tft.setTextColor(CAT_GREY, CAT_BG);
    tft.setTextDatum(ML_DATUM); 
    tft.drawString(String(minT), centerX + 5, startY + 58);
    
    tft.setTextDatum(MC_DATUM); 
  }
}