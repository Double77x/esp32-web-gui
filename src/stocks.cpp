#include "stocks.h"
#include "globals.h"    // For tft, colors, etc.
#include "config.h"     // For CAs
#include "drawing.h"    // For drawHeader, drawFooter, etc.
#include "utils.h"      // For HTTPSRequest, truncateDecimal
#include <ArduinoJson.h>
#include "Free_Fonts.h"
#include <secrets.h>

void fetchAndDisplayTicker(String ticker) {
  Serial.print("Fetching data for: ");
  Serial.println(ticker);

  drawHeader("Stocks");
  drawFooter(PAGE_STOCKS);
  drawStatusMessage("Fetching...", CAT_MUTED);

  String apiurl = "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + finnhub_api_key;
  
  String response = HTTPSRequest(apiurl, test_root_ca);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);

  String price;
  String change;
  uint16_t changeColor = CAT_RED;

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    price = "Error";
    change = "Parse Error";
  } else {
    double c = doc.containsKey("c") ? doc["c"].as<double>() : 0.0;
    double d = doc.containsKey("d") ? doc["d"].as<double>() : 0.0;
    double dp = doc.containsKey("dp") ? doc["dp"].as<double>() : 0.0;
    
    if (c == 0.0 && d == 0.0 && dp == 0.0) {
      price = "N/A";
      change = "No Data";
      changeColor = CAT_MUTED;
    } else {
      price = String(c, 2);
      change = String(d, 2);
      double dp_trunc = truncateDecimal((float)dp);
      change += " (" + String(dp_trunc, 2) + "%)";
      if (d >= 0) {
        change = "+" + change;
        changeColor = CAT_GREEN;
      } else {
        changeColor = CAT_RED;
      }
    }
  }

  // --- Start Drawing ---
  tft.fillScreen(CAT_BG);
  drawHeader("Stocks");
  drawFooter(PAGE_STOCKS);
  
  // drawTerminalFrame(); // <-- Removed this as requested

  tft.setTextDatum(MC_DATUM); 

  // --- Draw Ticker ---
  tft.setTextColor(CAT_MUTED, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  // Positioned neatly at the top
  tft.drawString(ticker, SCREEN_WIDTH / 2, HEADER_H + 35); 

  // --- Draw Price ---
  tft.setTextColor(CAT_TEXT, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB24);
  tft.setTextSize(1);
#else
  tft.setTextFont(8); 
  tft.setTextSize(1);
#endif
  // Positioned in the vertical center
  tft.drawString(price.startsWith("N/A") || price.startsWith("Error") ? price : "$" + price, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

  // --- Draw Change ---
  tft.setTextColor(changeColor, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  // Positioned neatly at the bottom
  tft.drawString(change, SCREEN_WIDTH / 2, SCREEN_HEIGHT - FOOTER_H - 35);
}