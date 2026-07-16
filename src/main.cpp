#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

MatrixPanel_I2S_DMA *display = nullptr;

void setup()
{
  delay(1000);
  Serial.begin(115200);

  const uint32_t psramBytes = ESP.getPsramSize();
  Serial.printf("Chip: %s\n", ESP.getChipModel());
  Serial.printf("Flash: %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("Usable PSRAM: %.2f MiB (%u bytes)\n", psramBytes / (1024.0 * 1024.0), psramBytes);

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
  display->setTextWrap(false);
  display->setTextSize(3);

  display->setTextColor(BVGColor);
  display->setCursor(2, 2);
  display->print("BVG");


  Serial.println("Display initialized successfully.");
}

void loop()
{
  Serial.println("Display is running.");
  delay(5000);
}
