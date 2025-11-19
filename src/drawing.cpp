#include "drawing.h"
#include "config.h"     // For screen dimensions, fonts
#include "globals.h"    // For tft, colors, currentSsid
#include "Free_Fonts.h"
#include <WiFi.h>       // For WiFi.localIP()

// =====================================================
// --- COLOR DEFINITIONS ---
// These create the actual variables in memory.
// =====================================================
uint16_t CAT_BG;
uint16_t CAT_TEXT;
uint16_t CAT_MUTED;
uint16_t CAT_ACCENT;
uint16_t CAT_SURFACE;
uint16_t CAT_GREEN;
uint16_t CAT_RED;

// New Colors for Weather/Stocks
uint16_t CAT_YELLOW;
uint16_t CAT_BLUE;
uint16_t CAT_WHITE;
uint16_t CAT_GREY;

// =====================================================
// --- INTERNAL HELPER ---
// Converts 24-bit hex (HTML style) to 16-bit 565 (TFT style)
// =====================================================
uint16_t color_from_hex(uint32_t hex) {
  uint8_t r = (hex >> 16) & 0xFF;
  uint8_t g = (hex >> 8) & 0xFF;
  uint8_t b = hex & 0xFF;
  return tft.color565(r, g, b);
}

// =====================================================
// --- INITIALIZE COLORS ---
// Call this in setup() before drawing anything!
// Using "Catppuccin Mocha" palette for modern look
// =====================================================
void init_colors() {
  // Base UI
  CAT_BG      = color_from_hex(0x1E1E2E); // Base
  CAT_TEXT    = color_from_hex(0xCDD6F4); // Text
  CAT_MUTED   = color_from_hex(0x6C7086); // Overlay0
  CAT_ACCENT  = color_from_hex(0xCBA6F7); // Mauve
  CAT_SURFACE = color_from_hex(0x181825); // Mantle (Darker header)

  // Functional Colors
  CAT_GREEN   = color_from_hex(0xA6E3A1); // Green
  CAT_RED     = color_from_hex(0xF38BA8); // Red
  
  // New Colors for Weather/Stocks
  CAT_YELLOW  = color_from_hex(0xF9E2AF); // Yellow
  CAT_BLUE    = color_from_hex(0x89B4FA); // Blue
  CAT_WHITE   = color_from_hex(0xFFFFFF); // Pure White
  CAT_GREY    = color_from_hex(0x9399B2); // Overlay1 (Lighter than muted)
}

// =====================================================
// --- DRAWING FUNCTIONS ---
// =====================================================

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

  // Draw Network Info on the right
  tft.setTextColor(CAT_MUTED, CAT_SURFACE);
  tft.setTextDatum(MR_DATUM);
  String networkInfo = currentSsid + " (" + WiFi.localIP().toString() + ")";
  tft.drawString(networkInfo, SCREEN_WIDTH - 8, HEADER_H / 2);
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

// Redraws just the network info part of the header after a reconnect
void updateHeaderIP() {
  // Clear the right side of the header
  tft.fillRect(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, HEADER_H, CAT_SURFACE);
  tft.drawLine(0, HEADER_H, SCREEN_WIDTH, HEADER_H, CAT_ACCENT); // Redraw line segment

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