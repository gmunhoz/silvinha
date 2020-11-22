#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <EEPROM.h>
#include <max6675.h>
#include <SD.h>
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
#define BUTTON_PIN           2
#define WATER_SENSOR_PIN     3
#define CARD_READER_CS_PIN   4
#define THERMOCOUPLE_CS_PIN  5
#define THERMOCOUPLE_SCK_PIN 6
#define THERMOCOUPLE_SO_PIN  7
#define TFT_DC_PIN           8
#define TFT_RST_PIN          9
#define TFT_CS_PIN          10

// Layout
#define DISPLAY_WIDTH      128
#define DISPLAY_HEIGHT     160
#define TOP_BAR_HEIGHT     40
#define BOTTOM_BAR_HEIGHT  40
#define TITLE_BAR_HEIGHT   10

// EEPROM
#define NUMBER_OF_BREWS_ADDRESS 0


/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  VARIABLES                            *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
MAX6675 thermocouple(THERMOCOUPLE_SCK_PIN, THERMOCOUPLE_CS_PIN, THERMOCOUPLE_SO_PIN);

unsigned long poweredAt = millis();
unsigned long millisecondsSincePowered = 0;
int secondsSincePowered = 0;

unsigned long lastTimerUpdate = millis();
unsigned long currentTimerUpdate;

int currentTime = 0;
int lastRenderedTime = -1;

int currentWeight = 0;
int lastRenderedWeight = -1;
int lastPlottedWeight = -1;

int currentFlow = 0;
int lastRenderedFlow = -1;
int lastPlottedFlow = -1;

int currentTemperature = 0;
int lastRenderedTemperature = -1;
int lastPlottedTemperature = -1;

byte xAxisHeight = 0;
byte minX = 0;
byte maxX = 40;

byte yAxisWidth = 0;
byte minY = 0;
byte maxY = 100;

bool running = false;
bool lowWaterLevel = false;

int numberOfBrews = 0;

File brewFile;


/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  SETUP                                *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void setup() {
  Serial.begin(9600);
  setupPins();
  setupDisplay();
  setupCardReader();
  setupInitialState();
  drawLayout();
  plotPreviousBrewFromSD();
  delay(250);
}

void setupPins() {
  pinMode(BUTTON_PIN, INPUT);  
  pinMode(WATER_SENSOR_PIN, INPUT);
}

void setupDisplay() {
  display.initR(INITR_BLACKTAB);
  display.fillScreen(BLACK);
  display.setTextWrap(false);
}

void setupCardReader() {
  if (!SD.begin(CARD_READER_CS_PIN)) {
    Serial.println("Card reader initialization failed!");
    while (1);
  }
}

void setupInitialState() {
  numberOfBrews = max(readIntFromEEPROM(NUMBER_OF_BREWS_ADDRESS), 0);
  Serial.print("NUMBER OF BREWS: ");
  Serial.println(numberOfBrews);
}

/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  LOOPS                                *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void loop() {
  millisecondsSincePowered = millis() - poweredAt;
  int seconds = millisecondsSincePowered / 1000;

  if (seconds > secondsSincePowered) {
    secondsSincePowered = seconds;

    if (!lowWaterLevel) {
      oneSecondLoop();
    }
  }

  if (!lowWaterLevel) {
    fastLoop();
  }

  checkForInputs();
}

void fastLoop() {
  if (running) {
    updateTime();
  }

  drawTimePanel();
}

void oneSecondLoop() {
  if (running) {
    updateWeight();
    updateFlow();

    plotItem(currentTime, currentTemperature, currentFlow, currentWeight);
    writeItemToSD(currentTime, currentTemperature, currentFlow, currentWeight);

    if (currentTime >= maxX) {
      toggleRunning();
    }
  }
  
  updateTemperature();
  
  drawWeightPanel();
  drawFlowPanel();
  drawTemperaturePanel();
}


/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  EVENTS                               *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void checkForInputs() {
//  bool currentWaterLevel = digitalRead(WATER_SENSOR_PIN) == LOW;
//
//  // Waits for the current brew to finish before displaying the water level warning
//  if (lowWaterLevel != currentWaterLevel && !running) {
//    toggleLowWaterLevel();
//    delay(500);
//  }

  bool buttonPressed = digitalRead(BUTTON_PIN) == HIGH;

  // Only allows to update running flag if the water level is fine
  if (buttonPressed && !lowWaterLevel) {
    toggleRunning();
    delay(500);
  } 
}

void toggleLowWaterLevel() {
  if (lowWaterLevel) {
    resetPreviousRenderedValues();
    drawLayout();
  } else {
    drawWaterLevelWarning();
  }
  
  lowWaterLevel = !lowWaterLevel;
}

void resetPreviousRenderedValues() {
  lastRenderedTime = -1;
  lastRenderedWeight = -1;
  lastRenderedFlow = -1;
  lastRenderedTemperature = -1;
}

void toggleRunning() {
  if (!running) {
    String path = getBrewFileName(numberOfBrews);
    brewFile = SD.open(path, FILE_WRITE);
    
    clearGraph();
    
    lastTimerUpdate = millis();
    
    currentTime = 0;
    currentWeight = 0;
    currentFlow = 0;
    currentTemperature = 0;
    
  } else {
    brewFile.close();

    numberOfBrews++;
    writeIntIntoEEPROM(NUMBER_OF_BREWS_ADDRESS, numberOfBrews);
  }

  running = !running;
}



/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  UI Drawing                           *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */
 
void drawLayout() {
  float halfDisplayWidth = DISPLAY_WIDTH / 2;

  // Background
  display.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, BLACK);

  // Top bar
  drawPanel(0, 0, halfDisplayWidth, TOP_BAR_HEIGHT, "timer",  WHITE);
  drawPanel(halfDisplayWidth, 0, halfDisplayWidth, TOP_BAR_HEIGHT, "weight", YELLOW);

  // Graph
  drawGraph(0, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT, WHITE);

  // Bottom bar
  drawPanel(0, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, halfDisplayWidth, BOTTOM_BAR_HEIGHT, "flow", CYAN);
  drawPanel(halfDisplayWidth, DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT, halfDisplayWidth, BOTTOM_BAR_HEIGHT, "temp", RED);
}

void drawWaterLevelWarning() {
  display.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, RED);
  drawText("LOW WATER", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, REGULAR_TEXT, CENTERED, WHITE);
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
  drawText(maxXLabel, x + width, y - xAxisHeight + labelPadding, SMALL_TEXT, RIGHT_ALIGNED, BLACK);
  
  // Draw Y Axis
  display.fillRect(x, y - height, yAxisWidth, height - xAxisHeight, color);
  drawText(minYLabel, x + yAxisWidth, y - xAxisHeight - textHeight, SMALL_TEXT, RIGHT_ALIGNED, BLACK);
  drawText(mediumYLabel, x + yAxisWidth, y - (height / 2) - xAxisHeight + (textHeight / 2), SMALL_TEXT, RIGHT_ALIGNED, BLACK);
  drawText(maxYLabel, x + yAxisWidth, y - height + labelPadding, SMALL_TEXT, RIGHT_ALIGNED, BLACK);
}

void drawTimePanel() {
  if (currentTime != lastRenderedTime) {
    String text = getTimeText();
    fillCenteredRect(DISPLAY_WIDTH / 4, TITLE_BAR_HEIGHT + (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, (DISPLAY_WIDTH / 2) - 2, TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT - 2, BLACK);
    drawText(text, DISPLAY_WIDTH / 4, TITLE_BAR_HEIGHT + (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, REGULAR_TEXT, CENTERED, WHITE);
    lastRenderedTime = currentTime;
  }
}

void drawWeightPanel() {
  if (currentWeight != lastRenderedWeight) {
    String text = getWeightText();
    fillCenteredRect(DISPLAY_WIDTH - (DISPLAY_WIDTH / 4), TITLE_BAR_HEIGHT + (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, (DISPLAY_WIDTH / 2) - 2, TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT - 2, BLACK);
    drawText(text, DISPLAY_WIDTH - (DISPLAY_WIDTH / 4), TITLE_BAR_HEIGHT + (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, REGULAR_TEXT, CENTERED, WHITE);
    lastRenderedWeight = currentWeight;
  }
}

void drawFlowPanel() {
  if (currentFlow != lastRenderedFlow) {
    String text = getFlowText();
    fillCenteredRect(DISPLAY_WIDTH / 4, DISPLAY_HEIGHT - (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, (DISPLAY_WIDTH / 2) - 2, TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT - 2, BLACK);
    drawText(text, DISPLAY_WIDTH / 4, DISPLAY_HEIGHT - (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, REGULAR_TEXT, CENTERED, WHITE);
    lastRenderedFlow = currentFlow;
  }
}

void drawTemperaturePanel() {
  if (currentTemperature != lastRenderedTemperature) {
    String text = getTemperatureText();
    fillCenteredRect(DISPLAY_WIDTH - (DISPLAY_WIDTH / 4), DISPLAY_HEIGHT - (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, (DISPLAY_WIDTH / 2) - 2, TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT - 2, BLACK);
    drawText(text, DISPLAY_WIDTH - (DISPLAY_WIDTH / 4), DISPLAY_HEIGHT - (TOP_BAR_HEIGHT - TITLE_BAR_HEIGHT) / 2, REGULAR_TEXT, CENTERED, WHITE);
    lastRenderedTemperature = currentTemperature;
  }
}

void fillCenteredRect(float x, float y, float width, float height, unsigned short color) {
  display.fillRect(x - (width / 2), y - (height / 2), width, height, color);
}

void plotItem(byte seconds, byte temperature, byte flowRate, byte weight) {
  plotTemperature(seconds, temperature);
  plotFlowRate(seconds, flowRate);
  plotWeight(seconds, weight);
}

void plotTemperature(byte seconds, byte value) {
  if (seconds > 0) {
    byte x1 = seconds - 1;
    float y1 = lastPlottedTemperature;
    byte x2 = seconds;
    float y2 = value;
    plotData(x1, y1, x2, y2, RED);
  }

  lastPlottedTemperature = value;
}

void plotFlowRate(byte seconds, byte value) {
  if (seconds > 0) {
    byte x1 = seconds - 1;
    float y1 = lastPlottedFlow * 10;
    byte x2 = seconds;
    float y2 = value * 10;
    plotData(x1, y1, x2, y2, CYAN);
  }

  lastPlottedFlow = value;
}

void plotWeight(byte seconds, byte value) {
  if (seconds > 0) {
    byte x1 = seconds - 1;
    float y1 = lastPlottedWeight;
    byte x2 = seconds;
    float y2 = value;
    plotData(x1, y1, x2, y2, YELLOW);
  }

  lastPlottedWeight = value;
}

void plotData(float x1, float y1, float x2, float y2, int color) {
  float px =  graphXtoScreenX(x1);
  float py =  graphYtoScreenY(y1);
  
  float x =  graphXtoScreenX(x2);
  float y =  graphYtoScreenY(y2);
  
  display.drawLine(px, py, x, y, color);
  display.drawLine(px, py - 1, x, y - 1, color);
  display.drawLine(px, py + 1, x, y + 1, color);
}

void plotPreviousBrewFromSD() {
  if (numberOfBrews > 0) {
    plotBrewFromSD(numberOfBrews - 1);
  }
}

void plotBrewFromSD(int brewNumber) {
  String path = getBrewFileName(brewNumber);

  Serial.print("PATH: ");
  Serial.println(path);
  
  brewFile = SD.open(path);

  while (brewFile.available()) {
    byte seconds = brewFile.parseInt();
    byte temperature = brewFile.parseInt();
    byte flowRate = brewFile.parseInt();
    byte weight = brewFile.parseInt();

    plotItem(seconds, temperature, flowRate, weight);
  }

  brewFile.close();
}

 void clearGraph() {
  float graphHeight = DISPLAY_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT;
  display.fillRect(yAxisWidth, TOP_BAR_HEIGHT, DISPLAY_WIDTH - yAxisWidth - 1, graphHeight - xAxisHeight, BLACK);
}

void drawText(const String &text, float x, float y, byte size, byte alignment, unsigned short color) {
  int16_t x1, y1;
  uint16_t w, h;
  
  display.setTextSize(size);             
  display.setTextColor(color);      
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);

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
  
  display.print(text);
}


/* * * * * * * * * * * * * * * * * * * * *
 *                                       *
 *  HELPERS                              *
 *                                       *
 * * * * * * * * * * * * * * * * * * * * */

void updateTime() {
  currentTimerUpdate = millis();
  currentTime = (currentTimerUpdate - lastTimerUpdate) / 1000;
}

void updateWeight() {
  // TODO: read the actual weight
  currentWeight += rand() % 3; 
}

void updateFlow() {
  // TODO: calculate the actual flow
  currentFlow = rand() % 3 + 1;
}

void updateTemperature() {
  currentTemperature = thermocouple.readCelsius();
}

String getTimeText() {
  char text[4];
  sprintf(text, "%01ds", currentTime);
  return String(text);
}

String getTemperatureText() {
  char text[4];
  sprintf(text, "%01dc", currentTemperature);
  return String(text);
}

String getFlowText() {
  char text[4];
  sprintf(text, "%01dg/s", currentFlow);
  return String(text);
}

String getWeightText() {
  char text[4];
  sprintf(text, "%01dg", currentWeight);
  return String(text);
}

String getBrewFileName(int brewNumber) {
  char text[10];
  sprintf(text, "%05d.txt", brewNumber);
  return String(text);
}

double graphXtoScreenX(double x) {
  float graphWidth = DISPLAY_WIDTH;
  float plottingWidth = graphWidth - yAxisWidth - 2;
  return ((x - minY) * (plottingWidth) / (maxX - minX)) + yAxisWidth;
}

double graphYtoScreenY(double y) {
  float graphHeight = DISPLAY_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT;
  float graphY = DISPLAY_HEIGHT - BOTTOM_BAR_HEIGHT;
  float plottingHeight = (graphHeight - xAxisHeight - 2);
  return ((y - minY) * (graphY - plottingHeight - graphY) / (maxY - minY) + graphY) - xAxisHeight - 2;
}

void writeItemToSD(byte seconds, byte temperature, byte flowRate, byte weight) {
  char text[15];
  sprintf(text, "%03d,%03d,%03d,%03d", seconds, temperature, flowRate, weight);
  brewFile.println(text);
}

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}
