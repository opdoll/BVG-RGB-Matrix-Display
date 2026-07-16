#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>

#include "api_client.h"
#include "secrets.h"

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

MatrixPanel_I2S_DMA *display = nullptr;

void setup()
{
  delay(1000);
  Serial.begin(115200);

  // wifi setup
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  for (int attempt = 0;
       attempt < 40 && WiFi.status() != WL_CONNECTED;
       ++attempt) {
    delay(500);
    Serial.print(".");
       }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
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

  display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!display->begin()) {
    Serial.println("Display initialization failed");
    while (true) {
      delay(1000);
    }
  }

  uint16_t BVGColor = MatrixPanel_I2S_DMA::color565(254, 163, 1);

  display->setBrightness8(100);
  display->clearScreen();
  display->setTextSize(1);

  display->setTextColor(BVGColor);
  display->setCursor(4, 4);
  display->print("Waiting for api!");


  Serial.println("Display initialized successfully.");
}

String currentMessage;

void loop()
{
  String newMessage;

  if (fetchMessage(newMessage) && newMessage != currentMessage) {
    currentMessage = newMessage;

    Serial.print("Updating display text to: ");
    Serial.println(currentMessage);

    display->clearScreen();
    display->setCursor(4, 4);
    display->print(currentMessage);
  }

  delay(5000);
}
