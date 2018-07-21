#include <Wire.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include "images.h"

SSD1306Wire display(0x3C,25,26,GEOMETRY_128_32);
OLEDDisplayUi ui ( &display );

unsigned long startTime;

int nFuelLevel;
bool bBurn;
int nWiFiLevel;

void drawOverlay1(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  if (nFuelLevel<10) display->drawXbm(0, 0, FuelBig_width, FuelBig_height, FuelBig_bits);
  else if (bBurn) display->drawXbm(0, 0, Fire_Logo_width, Fire_Logo_height, Fire_Logo_bits);
  else
  {
    
  }
}

void drawOverlay2(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  display->drawXbm(50, 0, FuelMini_width, FuelMini_height, FuelMini_bits);
  display->drawProgressBar(60,0,64,8,nFuelLevel);
}

void drawOverlay3(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  if (nWiFiLevel>70) display->drawXbm(35, 0, WiFi100_width, WiFi100_height, WiFi100_bits);
  else if (nWiFiLevel > 40) display->drawXbm(35, 0, WiFi50_width, WiFi50_height, WiFi50_bits);
  else display->drawXbm(35, 0, WiFi20_width, WiFi20_height, WiFi20_bits);
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_24);
  display->drawString(x+127, y+8, "23.7°C");
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_24);
  display->drawString(x+127, y+8, "997 hPa");
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_24);
  display->drawString(x+127, y+8, "67%");
}

// Array of frames
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3 };
int frameCount = 3;

// Array of overlays
OverlayCallback overlays[] = { drawOverlay1, drawOverlay2, drawOverlay3 };
int overlaysCount = 3;

void setup()
{
  Serial.begin(115200);

  ui.setTargetFPS(30);
  ui.disableAllIndicators();
  //ui.disableAutoTransition();
  ui.enableAutoTransition();
  ui.setFrameAnimation(SLIDE_RIGHT);
  
  // Add frames
  ui.setFrames(frames, frameCount);
  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();
  delay(100);
  
  nFuelLevel = 100;
  bBurn = false;
  nWiFiLevel = 100;
  randomSeed(analogRead(18));
  startTime = millis();
}

void loop()
{
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0)
  {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    //Serial.print("Remaining Time Budget = ");
    //Serial.print(remainingTimeBudget);
    //Serial.println(" ms");
    if ((millis()-startTime)>1000)
    {
      startTime=millis();
      if (nFuelLevel-- <= 0) nFuelLevel = 100;
      if (nFuelLevel > 70) bBurn = true;
      if (nFuelLevel < 20) bBurn = false;
      nWiFiLevel = random(10,100);
    }
  }
}
