#include "stocks.h"
#include "globals.h"    // For tft, colors, etc.
#include "config.h"     // For CAs
#include "secrets.h"    // For finnhub_api_key
#include "drawing.h"    // For drawHeader, drawFooter, etc.
#include "utils.h"      // For HTTPSRequest, truncateDecimal
#include <ArduinoJson.h>
#include "Free_Fonts.h"
#include <time.h>       // For getting timestamps

// =========================================================================
// --- NEW HELPER FUNCTION: High/Low/Current Price Bar ---
// =========================================================================
/**
 * @brief Draws a horizontal bar chart showing the day's low, high, and current price.
 * @param low The day's low price.
 * @param high The day's high price.
 * @param current The current price.
 * @param color The color for the current price indicator (green/red).
 */
void drawPriceBar(float low, float high, float current, uint16_t color) {
  // 1. Define chart boundaries
  int chartX = 40;
  int chartW = SCREEN_WIDTH - (chartX * 2); // 240px wide
  int chartY = 185; // <-- Y position for the chart

  // Avoid division by zero if high and low are the same
  if (high == low) {
    low -= 0.01; // Prevent division by zero
  }

  // 2. Draw the main horizontal line
  tft.drawFastHLine(chartX, chartY, chartW, CAT_MUTED);

  // 3. Draw end ticks for Low and High
  tft.drawFastVLine(chartX, chartY - 5, 11, CAT_MUTED); // Low tick
  tft.drawFastVLine(chartX + chartW, chartY - 5, 11, CAT_MUTED); // High tick

  // 4. Draw labels for Low and High
  tft.setTextColor(CAT_MUTED, CAT_BG);
  #if USE_FREE_FONTS
    tft.setFreeFont(FSS9);
    tft.setTextSize(1);
  #else
    tft.setTextFont(2);
    tft.setTextSize(1);
  #endif
  
  tft.setTextDatum(TC_DATUM); // Top-Center alignment
  tft.drawString(String(low, 2), chartX, chartY + 10); // Label below tick
  
  tft.setTextDatum(TC_DATUM); // Top-Center alignment
  tft.drawString(String(high, 2), chartX + chartW, chartY + 10); // Label below tick

  // 5. Calculate and draw the current price dot
  float range = high - low;
  float norm_price = (current - low) / range;
  
  // Clamp value between 0.0 and 1.0 in case current is outside H/L
  if (norm_price < 0.0) norm_price = 0.0;
  if (norm_price > 1.0) norm_price = 1.0;
  
  int dot_x = chartX + (int)(norm_price * chartW);
  
  // Draw a circle for the current price
  tft.fillCircle(dot_x, chartY, 6, color);
  tft.drawCircle(dot_x, chartY, 6, CAT_TEXT); // White outline
}

// =========================================================================
// --- UPDATED: Main Fetch & Display Function ---
// =========================================================================
void fetchAndDisplayTicker(String ticker) {
  Serial.print("Fetching data for: ");
  Serial.println(ticker);

  drawHeader("Stocks"); // <-- FIX: Changed title
  drawFooter(PAGE_STOCKS);
  drawStatusMessage("Fetching...", CAT_MUTED);

  // --- Step 1: Get CURRENT Quote ---
  // This one API call has all the data we need (c, h, l, d, dp)
  String quoteUrl = "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + String(finnhub_api_key);
  String quoteResponse = HTTPSRequest(quoteUrl, test_root_ca);

  // --- Add verbose logging for debugging quote ---
  Serial.println("--- Finnhub Quote Response ---");
  Serial.println(quoteResponse);
  Serial.println("--------------------------------");

  DynamicJsonDocument quoteDoc(512); // Use Dynamic for safety
  DeserializationError error = deserializeJson(quoteDoc, quoteResponse);

  String price;
  String change;
  uint16_t changeColor = CAT_RED;
  float val_h = 0.0, val_l = 0.0, val_c = 0.0;
  bool dataValid = false;

  if (error) {
    Serial.print("JSON parse error (Quote): ");
    Serial.println(error.c_str());
    price = "Error";
    change = "Parse Error";
  } else {
    // Parse all values from the single response
    val_c = quoteDoc.containsKey("c") ? quoteDoc["c"].as<double>() : 0.0; // Current
    val_h = quoteDoc.containsKey("h") ? quoteDoc["h"].as<double>() : 0.0; // High
    val_l = quoteDoc.containsKey("l") ? quoteDoc["l"].as<double>() : 0.0; // Low
    double d = quoteDoc.containsKey("d") ? quoteDoc["d"].as<double>() : 0.0; // Change
    double dp = quoteDoc.containsKey("dp") ? quoteDoc["dp"].as<double>() : 0.0; // Pct Change
    
    if (val_c == 0.0 && val_h == 0.0 && val_l == 0.0) {
      price = "N/A";
      change = "No Data";
      changeColor = CAT_MUTED;
      dataValid = false;
    } else {
      dataValid = true;
      price = String(val_c, 2);
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

  // --- Step 2: Start Drawing ---
  tft.fillScreen(CAT_BG);
  drawHeader("Stocks"); // <-- FIX: Changed title
  drawFooter(PAGE_STOCKS);
  
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
  tft.drawString(ticker, SCREEN_WIDTH / 2, 65); // Y=65

  // --- Draw Price ---
  tft.setTextColor(CAT_TEXT, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB24);
  tft.setTextSize(1);
#else
  tft.setTextFont(8); 
  tft.setTextSize(1);
#endif
  // Positioned as the main element
  tft.drawString(price.startsWith("N/A") || price.startsWith("Error") ? price : "$" + price, SCREEN_WIDTH / 2, 105); // Y=105

  // --- Draw Change ---
  tft.setTextColor(changeColor, CAT_BG);
#if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  // Positioned right below the price
  tft.drawString(change, SCREEN_WIDTH / 2, 140); // Y=140

  // --- Draw H/L/C Price Bar (NEW) ---
  if (dataValid) {
    drawPriceBar(val_l, val_h, val_c, changeColor);
  } else {
    Serial.println("Skipping price bar: Data is invalid or N/A.");
  }
}