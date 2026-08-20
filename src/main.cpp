#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>

#include "api_client.h"
#include "font_3x5.h"

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

constexpr char WIFI_CONFIG_AP_NAME[] = "BVG-Display-Setup";
constexpr char DEFAULT_API_BASE_URL[] = "https://v6.vbb.transport.rest/stops/";
constexpr char DEFAULT_STOP_ID[] = "900086107";
constexpr char DEFAULT_LINES[] = "221,125";
constexpr char DEFAULT_DIRECTIONS[] = "U Kurt-Schumacher-Platz,U Leopoldplatz,U Osloer Str.";
constexpr unsigned long DEPARTURE_UPDATE_INTERVAL = 20000;

MatrixPanel_I2S_DMA *display = nullptr;
WiFiManager wifiManager;
WiFiManagerParameter stopIdParameter("stopId", "Stop ID", DEFAULT_STOP_ID, 256);
WiFiManagerParameter lineParameter("line", "Lines, comma-separated (empty for all)", DEFAULT_LINES, 256);
WiFiManagerParameter directionParameter("direction", "Directions, comma-separated (empty for all)", DEFAULT_DIRECTIONS, 256);
WiFiManagerParameter apiEndpointParameter("apiEndpoint", "Advanced: API endpoint", DEFAULT_API_BASE_URL, 256);

void saveDisplayConfig()
{
  Preferences preferences;
  preferences.begin("display", false);
  preferences.putString("stopId", stopIdParameter.getValue());
  preferences.putString("line", lineParameter.getValue());
  preferences.putString("direction", directionParameter.getValue());
  preferences.putString("apiEndpoint", apiEndpointParameter.getValue());
  preferences.end();
}

void loadDisplayConfig()
{
  Preferences preferences;
  preferences.begin("display", false);
  String stopId = preferences.getString("stopId", DEFAULT_STOP_ID);
  String line = preferences.getString("line", DEFAULT_LINES);
  String direction = preferences.getString("direction", DEFAULT_DIRECTIONS);
  String apiEndpoint = preferences.getString("apiEndpoint", DEFAULT_API_BASE_URL);
  preferences.end();

  stopIdParameter.setValue(stopId.c_str(), 256);
  lineParameter.setValue(line.c_str(), 256);
  directionParameter.setValue(direction.c_str(), 256);
  apiEndpointParameter.setValue(apiEndpoint.c_str(), 256);
}

void configModeCallback(WiFiManager *wifiManager)
{
  Serial.println("Wifi connection failed. Configuration portal started at ");
  Serial.print(wifiManager->getConfigPortalSSID());
  Serial.print(" and the address: http://");
  Serial.println(WiFi.softAPIP());
}

void wait()
{
  unsigned long startedAt = millis();
  while (millis() - startedAt < DEPARTURE_UPDATE_INTERVAL) {
    wifiManager.process();
    delay(10);
  }
}

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
  loadDisplayConfig();
  wifiManager.addParameter(&stopIdParameter);
  wifiManager.addParameter(&lineParameter);
  wifiManager.addParameter(&directionParameter);
  wifiManager.addParameter(&apiEndpointParameter);
  wifiManager.setParamsPage(true);
  wifiManager.setSaveParamsCallback(saveDisplayConfig);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConnectTimeout(20);
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.setShowDnsFields(true);

  if (!wifiManager.autoConnect(WIFI_CONFIG_AP_NAME)) {
    Serial.println("WiFi setup failed. Restarting.");
    delay(3000);
    ESP.restart();
  }

  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  wifiManager.startWebPortal();
  Serial.print("Config portal: http://");
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
  int departureCount = fetchDepartures(departures, apiEndpointParameter.getValue(), stopIdParameter.getValue(), lineParameter.getValue(), directionParameter.getValue());

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

  wait();
}
