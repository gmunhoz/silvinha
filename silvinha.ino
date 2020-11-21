#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>

// Colors
#define BLACK  0x0000
#define RED    0xF800
#define CYAN   0x07FF
#define YELLOW 0xFFE0
#define WHITE  0xFFFF
#define GRAY   0x2945

// Pins
#define TFT_DC_PIN  8
#define TFT_RST_PIN 9
#define TFT_CS_PIN  10

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

void setup() {
  Serial.begin(9600);
  setupDisplay();
  delay(250);
}

void setupDisplay() {
  display.initR(INITR_BLACKTAB);
  display.fillScreen(BLACK);
  display.setTextWrap(false);
}

void loop() {}
