#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <time.h>

#include "api_client.h"
#include "font_3x5.h"
#include "secrets.h"

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

MatrixPanel_I2S_DMA *display = nullptr;

uint16_t textWidth(const String &text)
{
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
  display->getTextBounds(text, 0, 0, &x, &y, &width, &height);
  return width;
}

String fitToWidth(String text, uint16_t maxWidth)
{
  while (!text.isEmpty() && textWidth(text) > maxWidth) {
    text.remove(text.length() - 1);
  }

  return text;
}

String normalizeText(String text) // use replacement chars for now
{
  text.replace("ä", "{");
  text.replace("Ä", "{");
  text.replace("ö", "|");
  text.replace("Ö", "|");
  text.replace("ü", "}");
  text.replace("Ü", "}");
  text.replace("ß", "SS");
  text.toUpperCase();
  return text;
}

void showDebugMessage(const char *message)
{
  display->clearScreen();
  display->setCursor(2, 5);
  display->print(message);
}

void drawDepartures(const Departure departures[DEPARTURE_COUNT], int departureCount)
{
  const uint16_t BVGColor = MatrixPanel_I2S_DMA::color565(254, 163, 1);

  display->clearScreen();
  display->setTextColor(BVGColor);

  const String departureTitle = "ABFAHRT";
  const int16_t departureTitleX = PANEL_RES_X * PANEL_CHAIN - 2 - textWidth(departureTitle);

  display->setCursor(2, 6);
  display->print("LINIE");

  display->setCursor(26, 6);
  display->print("RICHTUNG");

  display->setCursor(departureTitleX, 6);
  display->print(departureTitle);

  for (int index = 0; index < departureCount; ++index) {
    const int16_t y = 15 + index * 7;
    const String mins = String(departures[index].mins);
    const int16_t minsX = PANEL_RES_X * PANEL_CHAIN - 2 - textWidth(mins);

    String direction = normalizeText(departures[index].direction);
    direction = fitToWidth(direction, minsX - 28);

    display->setCursor(2, y);
    display->print(departures[index].line);

    display->setCursor(26, y);
    display->print(direction);

    display->setCursor(minsX, y);
    display->print(mins);
  }
}

void setup()
{
  delay(1000);
  Serial.begin(115200);

  // wifi setup
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  for (int attempt = 0; attempt < 40 && WiFi.status() != WL_CONNECTED; ++attempt) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());

    configTzTime(
      "CET-1CEST,M3.5.0,M10.5.0/3",
      "pool.ntp.org",
      "time.nist.gov"
    );

    tm timeInfo;
    if (getLocalTime(&timeInfo, 10000)) {
      Serial.println("Time synchronized");
    } else {
      Serial.println("Time synchronization failed");
    }
  } else {
    Serial.println("WiFi connection failed");
  }

  // adapt to pin layout of rgb matrix
  HUB75_I2S_CFG::i2s_pins pins = {
    4, 6, 5,
    7, 16, 15,
    18, 8, 3, 42, 9,
    40, 2, 41
  };

  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN,
    pins
  );

  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;
  mxconfig.clkphase = false;

  display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!display->begin()) {
    Serial.println("Display initialization failed");
    while (true) {
      delay(1000);
    }
  }

  uint16_t BVGColor = MatrixPanel_I2S_DMA::color565(254, 163, 1);

  display->setBrightness8(50);
  display->clearScreen();
  display->setTextWrap(false);
  display->setFont(&Font3x5);
  display->setTextSize(1);

  display->setTextColor(BVGColor);
  display->setCursor(2, 6);
  display->print("WAITING FOR API");

  Serial.println("Display initialized successfully.");
}

void loop()
{
  Departure departures[DEPARTURE_COUNT];
  int departureCount = fetchDepartures(departures);

  switch (departureCount) {
    case 0:
      showDebugMessage("NO DEPARTURES");
      break;
    case NO_WIFI_CONNECTION:
      showDebugMessage("NO WIFI CONNECTION");
      break;
    case API_ERROR:
      showDebugMessage("API ERROR");
      break;
    default:
      drawDepartures(departures, departureCount);
      break;
  }

  delay(5000);
}
