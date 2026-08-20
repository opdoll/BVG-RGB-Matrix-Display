#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "api_client.h"

constexpr char API_DEPARTURES_QUERY[] = "/departures?results=20&duration=60&remarks=false&linesOfStops=false&pretty=false";

static WiFiClientSecure client;
static HTTPClient http;
static bool clientReady = false;

void prepareClient() {
    if (clientReady) {
        return;
    }

    client.setInsecure();
    client.setHandshakeTimeout(10);
    http.setReuse(true);
    http.setTimeout(15000);
    clientReady = true;
}

bool matchesFilter(const String &value, const String &filter) {
    String list = "," + filter + ",";
    return filter.isEmpty() || list.indexOf("," + value + ",") >= 0;
}

bool minutesUntil(const char *timestamp, int &minutes) {

    tm departureTime = {};
    departureTime.tm_isdst = -1;

    if (strptime(timestamp, "%Y-%m-%dT%H:%M:%S", &departureTime) == nullptr) {
        return false;
    }

    const time_t departure = mktime(&departureTime);
    const time_t now = time(nullptr);

    const long seconds = departure - now;
    minutes = seconds <= 0 ? 0 : static_cast<int>((seconds + 59) / 60);
    return true;
}

int fetchDepartures(Departure departures[DEPARTURE_COUNT], const String &apiBaseUrl, const String &stopId, const String &wantedLines, const String &wantedDirections) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("API request skipped: WiFi is disconnected");
        return NO_WIFI_CONNECTION;
    }

    prepareClient();

    String departuresUrl = apiBaseUrl + stopId + API_DEPARTURES_QUERY;
    if (!http.begin(client, departuresUrl)) {
        Serial.println("API request failed: invalid URL");
        return API_ERROR;
    }

    int status = http.GET();

    if (status < 0) {
        Serial.printf("API request failed: HTTP %d (%s)\n", status, HTTPClient::errorToString(status).c_str());
        http.end();
        client.stop();
        return API_ERROR;
    }

    if (status != HTTP_CODE_OK) {
        Serial.printf("API returned HTTP %d\n", status);
        http.end();
        client.stop();
        return API_ERROR;
    }

    String response = http.getString();
    http.end();

    JsonDocument filter;
    filter["departures"][0]["line"]["name"] = true;
    filter["departures"][0]["direction"] = true;
    filter["departures"][0]["when"] = true;
    filter["departures"][0]["plannedWhen"] = true;

    JsonDocument document;
    DeserializationError error = deserializeJson(document, response, DeserializationOption::Filter(filter));
    if (error) {
        Serial.printf("API returned invalid JSON: %s\n", error.c_str());
        client.stop();
        return API_ERROR;
    }

    JsonArray items = document["departures"].as<JsonArray>();
    int departureCount = 0;

    for (JsonObject item : items) {
        if (departureCount >= DEPARTURE_COUNT) {
            break;
        }

        String line = item["line"]["name"].as<String>();
        String direction = item["direction"].as<String>();
        if (!matchesFilter(line, wantedLines) || !matchesFilter(direction, wantedDirections)) {
            continue;
        }

        const char *timestamp = item["when"];
        if (timestamp == nullptr) {
            timestamp = item["plannedWhen"];
        }

        int minutes;
        if (!minutesUntil(timestamp, minutes)) {
            Serial.println("API returned an invalid departure time");
            continue;
        }

        departures[departureCount].line = line;
        departures[departureCount].direction = direction;
        departures[departureCount].mins = minutes;
        ++departureCount;
    }

    return departureCount;
}
