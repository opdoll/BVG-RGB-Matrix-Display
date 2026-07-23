#pragma once
#include <Arduino.h>

constexpr int DEPARTURE_COUNT = 3;
constexpr int NO_API_CONNECTION = -1;
constexpr int EXTERNAL_API_FAILED = -2;
constexpr int API_ERROR = -3;

struct Departure {
    String line;
    String direction;
    int mins;
};

int fetchDepartures(Departure departures[DEPARTURE_COUNT]);
