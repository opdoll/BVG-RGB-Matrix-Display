#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#include "api_client.h"
#include "secrets.h"

int fetchDepartures(Departure departures[DEPARTURE_COUNT]) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API request skipped: WiFi is disconnected");
        return NO_API_CONNECTION;
    }

    WiFiClient client;
    HTTPClient http;

    if (!http.begin(client, API_URL)) {
        Serial.println("API request failed: invalid URL");
        return NO_API_CONNECTION;
    }

    int status = http.GET();

    if (status < 0) {
        Serial.printf("API request failed: HTTP %d (%s)\n", status, HTTPClient::errorToString(status).c_str());
        http.end();
        return NO_API_CONNECTION;
    }

    if (status != HTTP_CODE_OK) {
        Serial.printf("API returned HTTP %d\n", status);
        http.end();
        return status == 502 ? EXTERNAL_API_FAILED : API_ERROR;
    }

    String response = http.getString();
    http.end();

    JsonDocument document;
    if (deserializeJson(document, response)) {
        Serial.println("API returned invalid JSON");
        return API_ERROR;
    }

    JsonArray items = document["departures"].as<JsonArray>();
    int departureCount = min(static_cast<int>(items.size()), DEPARTURE_COUNT);

    for (int index = 0; index < departureCount; ++index) {
        departures[index].line = items[index]["line"].as<String>();
        departures[index].direction = items[index]["direction"].as<String>();
        departures[index].mins = items[index]["mins"].as<int>();
    }

    return departureCount;
}
