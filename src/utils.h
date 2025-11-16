#pragma once
#include <Arduino.h>

// Helper functions
String HTTPSRequest(String url, const char* root_ca);
void to_upper(const char *str, char *out_str);
float truncateDecimal(float value);