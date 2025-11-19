#include "stocks.h"
#include "globals.h"    // For tft, colors, etc.
#include "config.h"     // For CAs
#include "secrets.h"    // For finnhub_api_key
#include "drawing.h"    // For drawHeader, drawFooter, etc.
#include "utils.h"      // For HTTPSRequest, truncateDecimal
#include <ArduinoJson.h>
#include "Free_Fonts.h"

//  - Visualizing a layout with huge price, a grid for Open/Prev, and a progress bar for the day's range.

// --- HELPER: Draw High/Low/Current Price Bar ---
void drawPriceBar(float low, float high, float current, uint16_t color) {
  // --- ADJUSTMENTS HERE ---
  // Increased barX from 60 to 75 to shorten the bar width
  // Moved barY from 205 to 190 to move it away from the footer
  int barX = 75; 
  int barY = 190; 
  int barW = SCREEN_WIDTH - (barX * 2); // Width is calculated based on margins
  int barH = 6; // Thickness of the bar

  // 1. Draw Labels (Low on left, High on right)
  tft.setTextColor(CAT_MUTED, CAT_BG);
  #if USE_FREE_FONTS
    tft.setFreeFont(FSS9);
  #else
    tft.setTextFont(2);
  #endif
  
  // Draw Low Price (Right-aligned to the start of the bar)
  tft.setTextDatum(MR_DATUM); 
  tft.drawString(String(low, 2), barX - 8, barY + 3);

  // Draw High Price (Left-aligned to the end of the bar)
  tft.setTextDatum(ML_DATUM); 
  tft.drawString(String(high, 2), barX + barW + 8, barY + 3);

  // 2. Draw the Background Bar (Pill shape)
  tft.fillRoundRect(barX, barY, barW, barH, 3, CAT_MUTED);

  // 3. Calculate Position of the dot
  if (high == low) high += 0.01; // Prevent Div/0
  float range = high - low;
  float percent = (current - low) / range;
  
  // Clamp values to ensure dot stays inside the bar
  if (percent < 0.0) percent = 0.0;
  if (percent > 1.0) percent = 1.0;

  int indicatorX = barX + (int)(percent * barW);

  // 4. Draw Current Price Indicator
  // The "CAT_BG" outline creates a clean separation "cutout" effect
  tft.fillCircle(indicatorX, barY + 3, 6, color); 
  tft.drawCircle(indicatorX, barY + 3, 6, CAT_BG); 
}

// --- MAIN FUNCTION ---
void fetchAndDisplayTicker(String ticker) {
  Serial.print("Fetching data for: ");
  Serial.println(ticker);

  drawHeader("Stocks");
  drawFooter(PAGE_STOCKS);
  drawStatusMessage("Fetching quote...", CAT_MUTED);

  // --- Step 1: API Request ---
  // Finnhub Quote Endpoint: c=Current, h=High, l=Low, o=Open, pc=PrevClose, d=Change, dp=Percent
  String quoteUrl = "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + String(finnhub_api_key);
  String quoteResponse = HTTPSRequest(quoteUrl, test_root_ca);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, quoteResponse);

  // --- Step 2: Parse Data ---
  // Default values
  float current = 0.0, high = 0.0, low = 0.0, open = 0.0, prevClose = 0.0, change = 0.0, pctChange = 0.0;
  bool valid = false;

  if (error) {
    Serial.print("JSON Error: "); Serial.println(error.c_str());
    drawStatusMessage("JSON Error", CAT_RED);
  } else if (doc["c"].as<float>() == 0.0 && doc["h"].as<float>() == 0.0) {
    // API returns 0s for invalid tickers
    drawStatusMessage("Invalid Ticker", CAT_RED);
  } else {
    valid = true;
    current = doc["c"].as<float>();
    high = doc["h"].as<float>();
    low = doc["l"].as<float>();
    open = doc["o"].as<float>();
    prevClose = doc["pc"].as<float>();
    change = doc["d"].as<float>();
    pctChange = doc["dp"].as<float>();
  }

  // --- Step 3: Draw UI ---
  tft.fillScreen(CAT_BG);
  drawHeader("Stocks");
  drawFooter(PAGE_STOCKS);
  tft.setTextDatum(MC_DATUM);

  if (!valid) {
    tft.setTextColor(CAT_RED, CAT_BG);
    tft.drawString("Data Unavailable", SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    return;
  }

  // Determine Color (Green for up, Red for down)
  uint16_t color = (change >= 0) ? CAT_GREEN : CAT_RED;
  String sign = (change >= 0) ? "+" : "";

  // 3a. Ticker Symbol (Top)
  tft.setTextColor(CAT_MUTED, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB12);
  #else
  tft.setTextFont(4);
  #endif
  tft.drawString(ticker, SCREEN_WIDTH / 2, 55); 

  // 3b. Current Price (Huge, Center)
  tft.setTextColor(CAT_TEXT, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB24);
  #else
  tft.setTextFont(8);
  #endif
  tft.drawString("$" + String(current, 2), SCREEN_WIDTH / 2, 90);

  // 3c. Change & Percent (Below Price)
  String changeStr = sign + String(change, 2) + " (" + sign + String(pctChange, 2) + "%)";
  tft.setTextColor(color, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB12); // Bold medium
  #else
  tft.setTextFont(4);
  #endif
  tft.drawString(changeStr, SCREEN_WIDTH / 2, 125);

  // 3d. Secondary Info Grid (Open | Prev Close)
  int midY = 165;
  int leftX = SCREEN_WIDTH / 4;
  int rightX = (SCREEN_WIDTH / 4) * 3;

  // Labels
  tft.setTextColor(CAT_MUTED, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSS9); // Small
  #else
  tft.setTextFont(2);
  #endif
  tft.drawString("OPEN", leftX, midY);
  tft.drawString("PREV CLOSE", rightX, midY);

  // Values
  tft.setTextColor(CAT_TEXT, CAT_BG);
  #if USE_FREE_FONTS
  tft.setFreeFont(FSSB9); // Bold Small
  #else
  tft.setTextFont(2);
  #endif
  tft.drawString(String(open, 2), leftX, midY + 18);
  tft.drawString(String(prevClose, 2), rightX, midY + 18);

  // 3e. Day Range Bar (Bottom)
  drawPriceBar(low, high, current, color);
}