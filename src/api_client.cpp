#include <WiFiClient.h>
#include <HTTPClient.h>

#include "api_client.h"
#include "secrets.h"

bool fetchMessage(String &message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API request skipped: WiFi is disconnected");
        return false;
    }

    WiFiClient client;
    HTTPClient http;

    if (!http.begin(client, API_URL)) {
        Serial.println("API request failed: invalid URL");
        return false;
    }

    int status = http.GET();

    if (status != HTTP_CODE_OK) {
        Serial.printf("API request failed: HTTP %d (%s)\n", status, HTTPClient::errorToString(status).c_str());
        http.end();
        return false;
    }

    message = http.getString();
    message.trim();

    Serial.print("API message received: ");
    Serial.println(message);

    http.end();
    return true;
}
