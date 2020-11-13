#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display configuration
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Text configuration
#define TEXT_SIZE_SMALL 1
#define TEXT_SIZE_REGULAR 2
#define TEXT_SIZE_LARGE 3
#define TEXT_ALIGNMENT_LEFT 1
#define TEXT_ALIGNMENT_RIGHT 2
#define TEXT_ALIGNMENT_CENTER 3

// Water level configuration
#define FULL_WATER_CAPACITY 30
#define HALF_WATER_CAPACITY 15
#define LOW_WATER_CAPACITY 5

// Pin configuration
#define START_BUTTON 2
#define GREEN_LED 8

bool running = false;
int elapsedSecondsSinceLastRunningUpdate = 0;
double lastRunningUpdate = millis();
double currentRunningUpdate;

int elapsedSecondsSinceTimerStarted = 0;
double lastTimerUpdate = millis();
double currentTimerUpdate;

void setup() {
  Serial.begin(9600);
  pinMode(START_BUTTON, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  setupDisplay();  
}

void loop() {
  updateTimer();
  drawMainScreen();
  checkForActions();

  digitalWrite(GREEN_LED, running ? HIGH : LOW);  
}

String getTimerText() {
  char buf1[5];
  sprintf(buf1, "%01ds", elapsedSecondsSinceTimerStarted);
  return String(buf1);
}

void updateTimer() {
  if (running) {
    currentTimerUpdate = millis();
    elapsedSecondsSinceTimerStarted = (currentTimerUpdate - lastTimerUpdate) / 1000;
  }
}

void checkForActions() {
  if (digitalRead(START_BUTTON) == HIGH) {
      currentRunningUpdate = millis();
      elapsedSecondsSinceLastRunningUpdate = (currentRunningUpdate - lastRunningUpdate) / 1000;

      if (elapsedSecondsSinceLastRunningUpdate > 1) {
        toggleRunning();
      }
  } 
}

void toggleRunning() {
  running = !running;
  lastRunningUpdate = millis();

  if (running) {
    elapsedSecondsSinceTimerStarted = 0;
    lastTimerUpdate = millis();
  }
  
}

void setupDisplay() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

void drawMainScreen() {
  display.clearDisplay();

  // Draw water level
  drawProgressBar(0, 0, FULL_WATER_CAPACITY, 16, HALF_WATER_CAPACITY);
  drawText("water", 0, 16, TEXT_SIZE_SMALL, TEXT_ALIGNMENT_LEFT);

  // Draw temperature
  drawText("93C", SCREEN_WIDTH, 0, TEXT_SIZE_REGULAR, TEXT_ALIGNMENT_RIGHT);
  drawText("temp", SCREEN_WIDTH, 16, TEXT_SIZE_SMALL, TEXT_ALIGNMENT_RIGHT);

  // Timer
  String timerText = getTimerText();
  drawText(timerText, SCREEN_WIDTH/2, (SCREEN_HEIGHT/2)-4, TEXT_SIZE_REGULAR, TEXT_ALIGNMENT_CENTER);

  // Weight  
  drawText("41.5g", SCREEN_WIDTH/2, SCREEN_HEIGHT-16, TEXT_SIZE_REGULAR, TEXT_ALIGNMENT_CENTER);
  
  display.display();
}

void drawProgressBar(int x, int y, int w, int h, int fw) {
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  display.fillRect(x, y, fw, h, SSD1306_WHITE);
}

void drawText(const String &buf, int x, int y, int size, int alignment)
{
    int16_t x1, y1;
    uint16_t w, h;
    
    display.setTextSize(size);             
    display.setTextColor(SSD1306_WHITE);      
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);

    switch(alignment) {
      case 3:
        display.setCursor(x - w / 2, y);
        break; 
      case 2:
        display.setCursor(x - w, y);
        break;
      default :
        display.setCursor(x, y);
    }

    display.print(buf);
}
