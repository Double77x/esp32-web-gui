#pragma once
#include "globals.h"

// Function prototypes
uint16_t color_from_hex(uint32_t hex);
void init_colors();
void drawHeader(String title);
void updateHeaderIP(); // <-- NEW FUNCTION
void drawFooter(Page page);
void drawStatusMessage(const String& msg, uint16_t color);
void drawTerminalFrame();