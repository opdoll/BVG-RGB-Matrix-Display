#pragma once
#include <Arduino.h>

constexpr int DEPARTURE_COUNT = 3;
constexpr int API_ERROR = -1;
constexpr int NO_WIFI_CONNECTION = -2;

struct Departure {
    String line;
    String direction;
    int mins;
};

int fetchDepartures(Departure departures[DEPARTURE_COUNT]);
