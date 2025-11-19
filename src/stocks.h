#pragma once
#include <Arduino.h>

void fetchAndDisplayTicker(String ticker);

// Helper to draw the visual range bar
void drawPriceBar(float low, float high, float current, uint16_t color);