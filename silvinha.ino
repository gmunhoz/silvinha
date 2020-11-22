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

// Text Size
#define SMALL_TEXT   1
#define REGULAR_TEXT 2

// Text Alignment
#define LEFT_ALIGNED  1
#define RIGHT_ALIGNED 2
#define CENTERED      3

// Pins
#define TFT_DC_PIN  8
#define TFT_RST_PIN 9
#define TFT_CS_PIN  10

// Layout
#define DISPLAY_WIDTH      128
#define DISPLAY_HEIGHT     160
#define TOP_BAR_HEIGHT     40
#define BOTTOM_BAR_HEIGHT  40
#define TITLE_BAR_HEIGHT   10

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

byte xAxisHeight = 0;
byte minX = 0;
byte maxX = 40;

byte yAxisWidth = 0;
byte minY = 0;
byte maxY = 100;

double startTime = millis();
double millisecondsSinceStartTime = 0;
int secondsSinceStartTime = 0;

/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  SETUP                                *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void setup() {
  Serial.begin(9600);
  setupDisplay();
  setupLayout();
  delay(250);
}

void setupDisplay() {
  display.initR(INITR_BLACKTAB);
  display.fillScreen(BLACK);
  display.setTextWrap(false);
}

void setupLayout() {
  float halfDisplayWidth = DISPLAY_WIDTH / 2;

  // Top bar
  drawPanel(0, 0, halfDisplayWidth, TOP_BAR_HEIGHT, "timer",  WHITE);
  drawPanel(halfDisplayWidth, 0, halfDisplayWidth, TOP_BAR_HEIGHT, "weight", YELLOW);

  // Graph
  drawGraph(0, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT, WHITE);

  // Bottom bar
  drawPanel(0, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, halfDisplayWidth, BOTTOM_BAR_HEIGHT, "flow", CYAN);
  drawPanel(halfDisplayWidth, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, halfDisplayWidth, BOTTOM_BAR_HEIGHT, "temp", RED);
}

void drawPanel(float x, float y, float width, float height, String title, unsigned short color) {
  display.drawRect(x, y, width, height, color);
  display.fillRect(x, y, width, TITLE_BAR_HEIGHT, color);
  drawText(title, x + (width / 2), y + (TITLE_BAR_HEIGHT / 2), SMALL_TEXT, CENTERED, BLACK);
}

void drawGraph(float x, float y, float width, float height, unsigned short color) {
  uint16_t textWidth, textHeight;
  byte labelPadding = 1;

  // Prepare X Axis
  byte mediumX = (maxX - minX) / 2 + minX;

  String minXLabel = String(minX);
  String mediumXLabel = String(mediumX);
  String maxXLabel = String(maxX);

  display.getTextBounds(maxXLabel, 0, 0, 0, 0, &textWidth, &textHeight);
  xAxisHeight = labelPadding + textHeight + labelPadding;

  // Prepare Y Axis
  byte mediumY = (maxY - minY) / 2 + minY;
  
  String minYLabel = String(minY);
  String mediumYLabel = String(mediumY);
  String maxYLabel = String(maxY);

  display.getTextBounds(maxYLabel, 0, 0, 0, 0, &textWidth, &textHeight);
  yAxisWidth = labelPadding + textWidth + labelPadding;

  // Draw frame
  display.drawRect(x, y - height, width, height, color);

  // X Axis
  display.fillRect(x, y - xAxisHeight, width, xAxisHeight, color);
  drawText(minXLabel, x + yAxisWidth + labelPadding, y - xAxisHeight + labelPadding, SMALL_TEXT, LEFT_ALIGNED, BLACK);
  drawText(mediumXLabel, yAxisWidth + (width - yAxisWidth) / 2 + x, y - (xAxisHeight / 2), SMALL_TEXT, CENTERED, BLACK);
  drawText(maxXLabel, x + width, y - xAxisHeight + labelPadding, SMALL_TEXT, RIGHT_ALIGNED, BLACK); // TODO: fix this rataria
  
  // Draw Y Axis
  display.fillRect(x, y - height, yAxisWidth, height - xAxisHeight, color);
  drawText(minYLabel, x + yAxisWidth, y - xAxisHeight - textHeight, SMALL_TEXT, RIGHT_ALIGNED, BLACK);
  drawText(mediumYLabel, x + yAxisWidth, y - (height / 2) - xAxisHeight + (textHeight / 2), SMALL_TEXT, RIGHT_ALIGNED, BLACK);
  drawText(maxYLabel, x + yAxisWidth, y - height + labelPadding, SMALL_TEXT, RIGHT_ALIGNED, BLACK);
}


/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  LOOPS                                *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void loop() {
  millisecondsSinceStartTime = millis() - startTime;
  int seconds = millisecondsSinceStartTime / 1000;
  
  if (seconds > secondsSinceStartTime) {
    secondsSinceStartTime = seconds;
    oneSecondLoop();

    if (seconds % 2 == 0) {
      twoSecondsLoop();
    }
  }

  fastLoop();
}

void fastLoop() {
  
}

void oneSecondLoop() {
  
}

void twoSecondsLoop() {
  
}

 
/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  HELPERS                              *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void drawText(const String &buf, float x, float y, byte size, byte alignment, unsigned short color)
{
  int16_t x1, y1;
  uint16_t w, h;
  
  display.setTextSize(size);             
  display.setTextColor(color);      
  display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);

  switch(alignment) {
    case 3: 
      // Centered
      display.setCursor(x - w / 2, y - h / 2);
      break; 
    case 2: 
      // Right
      display.setCursor(x - w, y);
      break;
    default: 
      // Left
      display.setCursor(x, y);
  }

  display.print(buf);
}
