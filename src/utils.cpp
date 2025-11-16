#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "utils.h"
#include "config.h" // For ALLOW_INSECURE_TEST
#include "globals.h" // For Serial

// Move the function HTTPSRequest() from your .ino file here
String HTTPSRequest(String url, const char* root_ca) {
  String response = "";
  WiFiClientSecure client;

#if ALLOW_INSECURE_TEST
  client.setInsecure();
  Serial.println("WARNING: TLS certificate verification DISABLED (ALLOW_INSECURE_TEST=1)");
#else
  if (root_ca != nullptr) {
    client.setCACert(root_ca);
    Serial.println("Using embedded CA");
  } else {
    Serial.println("ERROR: No root CA provided!");
    return "Error: No CA";
  }
#endif

  HTTPClient http;
  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("HTTP GET code: %d\n", httpCode);
    response = http.getString();
  } else {
    Serial.printf("GET request failed, code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
  }

  http.end();
  return response;
}

// Move the function to_upper() from your .ino file here
void to_upper(const char *str, char *out_str)
{
  while(*str != 0) {
    *out_str = toupper(*str);
    ++str;
    ++out_str;
  }
  *out_str = 0;
}

// Move the function truncateDecimal() from your .ino file here
float truncateDecimal(float value) {
    return (int)(value * 100) / 100.0;
}