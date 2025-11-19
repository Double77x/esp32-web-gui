#pragma once
#include <Arduino.h>

void fetchAndDisplayWeather(String locationName);

// Helper functions (optional to expose, but good for debugging)
String getWeatherDescription(int code);
String getDayOfWeek(time_t unixtime);
void drawWeatherIcon(int x, int y, int code, int size, bool isNight = false);