#include "drawing.h"
#include "config.h"     // For screen dimensions, fonts
#include "globals.h"    // For tft, colors, currentSsid
#include "Free_Fonts.h"
#include <WiFi.h>       // For WiFi.localIP()

// Converts a 24-bit hex color to the 16-bit 565 format
uint16_t color_from_hex(uint32_t hex) {
  uint8_t r = (hex >> 16) & 0xFF;
  uint8_t g = (hex >> 8) & 0xFF;
  uint8_t b = hex & 0xFF;
  return tft.color565(r, g, b);
}

// Initializes the global color variables
void init_colors() {
  CAT_BG    = color_from_hex(0x1E1E2E);
  CAT_TEXT  = color_from_hex(0xDCE0E8);
  CAT_MUTED = color_from_hex(0x6E6C7E);
  CAT_ACCENT = color_from_hex(0xC6A0F6);
  CAT_GREEN = color_from_hex(0xABE9B3);
  CAT_RED   = color_from_hex(0xF28FAD);
  CAT_SURFACE = color_from_hex(0x11111B);
}

// Draws the top header bar with title and network status
void drawHeader(String title) {
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, CAT_SURFACE);
  tft.drawLine(0, HEADER_H, SCREEN_WIDTH, HEADER_H, CAT_ACCENT);

  tft.setTextColor(CAT_TEXT, CAT_SURFACE);
#if USE_FREE_FONTS
  tft.setFreeFont(FSS9);
  tft.setTextSize(1);
#else
  tft.setTextFont(1);
  tft.setTextSize(2);
#endif

  tft.setTextDatum(ML_DATUM);
  tft.drawString(" " + title, 8, HEADER_H / 2);

  // --- MODIFIED BLOCK ---
  tft.setTextColor(CAT_MUTED, CAT_SURFACE);
  tft.setTextDatum(MR_DATUM);
  // Show SSID and IP
  String networkInfo = currentSsid + " (" + WiFi.localIP().toString() + ")";
  tft.drawString(networkInfo, SCREEN_WIDTH - 8, HEADER_H / 2);
  // --- END MODIFIED BLOCK ---
}

// Draws the bottom footer bar with page toggle hint
void drawFooter(Page page) {
  tft.fillRect(0, SCREEN_HEIGHT - FOOTER_H, SCREEN_WIDTH, FOOTER_H, CAT_SURFACE);
  tft.drawLine(0, SCREEN_HEIGHT - FOOTER_H, SCREEN_WIDTH, SCREEN_HEIGHT - FOOTER_H, CAT_ACCENT);

  tft.setTextColor(CAT_MUTED, CAT_SURFACE);
#if USE_FREE_FONTS
  tft.setFreeFont(FSS9);
  tft.setTextSize(1);
#else
  tft.setTextFont(1);
  tft.setTextSize(2);
#endif

  tft.setTextDatum(MC_DATUM);

  String footer_text = "Touch for Weather";
  if (page == PAGE_WEATHER) {
    footer_text = "Touch for Stocks";
  }
  
  tft.drawString(footer_text, SCREEN_WIDTH / 2, SCREEN_HEIGHT - (FOOTER_H / 2));
}

// --- NEW FUNCTION ---
// Redraws just the network info part of the header after a reconnect
void updateHeaderIP() {
  // Clear the right side of the header
  tft.fillRect(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, HEADER_H, CAT_SURFACE);
  tft.setTextColor(CAT_MUTED, CAT_SURFACE);
#if USE_FREE_FONTS
  tft.setFreeFont(FSS9);
  tft.setTextSize(1);
#else
  tft.setTextFont(1);
  tft.setTextSize(2);
#endif
  tft.setTextDatum(MR_DATUM);
  String networkInfo = currentSsid + " (" + WiFi.localIP().toString() + ")";
  tft.drawString(networkInfo, SCREEN_WIDTH - 8, HEADER_H / 2);
}
// --- END NEW FUNCTION ---

// Draws a large status message in the center of the screen
void drawStatusMessage(const String& msg, uint16_t color) {
  tft.fillRect(0, HEADER_H + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_H - FOOTER_H - 1, CAT_BG);
  tft.setTextColor(color, CAT_BG);
  tft.setTextDatum(MC_DATUM);

#if USE_FREE_FONTS
  tft.setFreeFont(FSS12);
  tft.setTextSize(1);
#else
  tft.setTextFont(4);
  tft.setTextSize(1);
#endif
  
  tft.drawString(msg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
}

// Draws the decorative "terminal" frame for the stock page
void drawTerminalFrame() {
  int margin = 10;
  int lineY1 = HEADER_H + 45; 
  int lineY2 = SCREEN_HEIGHT - FOOTER_H - 45;
  
  tft.setTextFont(1);
  tft.setTextSize(2); 
  tft.setTextColor(CAT_MUTED, CAT_BG);

  tft.drawChar('+', margin, lineY1 - 4, 1); 
  tft.drawFastHLine(margin + 8, lineY1, SCREEN_WIDTH - (margin * 2) - 8, CAT_MUTED);
  tft.drawChar('+', SCREEN_WIDTH - margin - 8, lineY1 - 4, 1);

  tft.drawChar('+', margin, lineY2 - 4, 1);
  tft.drawFastHLine(margin + 8, lineY2, SCREEN_WIDTH - (margin * 2) - 8, CAT_MUTED);
  tft.drawChar('+', SCREEN_WIDTH - margin - 8, lineY2 - 4, 1);
}