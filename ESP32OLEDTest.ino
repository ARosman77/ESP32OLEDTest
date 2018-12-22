#include <Wire.h>
#include <SSD1306.h>
#include <BME280I2C.h>
#include <OLEDDisplayUi.h>
#include <Adafruit_APDS9960.h>
#include "images.h"

SSD1306Wire display(0x3C,25,26,GEOMETRY_128_32);
OLEDDisplayUi ui ( &display );
Adafruit_APDS9960 apds;

// BME280 sensor settings
BME280I2C::Settings settings(
   BME280::OSR_X16,
   BME280::OSR_X16,
   BME280::OSR_X16,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False,
   0x76 // I2C address. I2C specific.
);
BME280I2C bme(settings);

unsigned long startTime;

int nFuelLevel;
bool bBurn;
int nWiFiLevel;

void drawLeftLogo(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  if (nFuelLevel<10) display->drawXbm(0, 0, FuelBig_width, FuelBig_height, FuelBig_bits);
  else if (bBurn) display->drawXbm(0, 0, Fire_Logo_width, Fire_Logo_height, Fire_Logo_bits);
  else
  {
    
  }
}

void drawFuelLevel(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  display->drawXbm(50, 0, FuelMini_width, FuelMini_height, FuelMini_bits);
  display->drawProgressBar(60,0,64,8,nFuelLevel);
}

void drawWiFiLevel(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  if (nWiFiLevel>70) display->drawXbm(35, 0, WiFi100_width, WiFi100_height, WiFi100_bits);
  else if (nWiFiLevel > 40) display->drawXbm(35, 0, WiFi50_width, WiFi50_height, WiFi50_bits);
  else display->drawXbm(35, 0, WiFi20_width, WiFi20_height, WiFi20_bits);
}

void drawTemperatureFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  String output;
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_24);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  output = String(bme.temp(tempUnit),1) + "°C";
  display->drawString(x+127, y+8, output);
}

void drawPressureFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  String output;
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_24);
  BME280::PresUnit presUnit(BME280::PresUnit_hPa);
  output = String(bme.pres(presUnit),0) + "hPa";
  display->drawString(x+127, y+8, output);
}

void drawHumidityFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  String output;
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_24);
  output = String(bme.hum(),0) + "%";
  display->drawString(x+127, y+8, output);
}

// Array of frames
FrameCallback frames[] = { drawTemperatureFrame, drawPressureFrame, drawHumidityFrame };
int frameCount = 3;

// Array of overlays
OverlayCallback overlays[] = { drawLeftLogo, drawFuelLevel, drawWiFiLevel };
int overlaysCount = 3;

void setup()
{
  Serial.begin(115200);

  ui.setTargetFPS(30);
  ui.disableAllIndicators();
  ui.disableAutoTransition();
  //ui.enableAutoTransition();
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

  if(!apds.begin())
  {
    Serial.println("failed to initialize device! Please check your wiring.");
  }
  else
  {
    //gesture mode will be entered once proximity mode senses something close
    apds.enableProximity(true);
    apds.enableGesture(true);
    Serial.println("Device initialized!");
  }

  if(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
  }
  else
  {
    // bmeDataUpdater.attach(UPDATE_INTERVAL_s,bmeUpdateData);
  }
  
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
    //read a gesture from the device
    uint8_t gesture = apds.readGesture();
    if(gesture == APDS9960_DOWN) 
    {
      Serial.println("v");
      ui.previousFrame();
    }
    if(gesture == APDS9960_UP)
    {
      Serial.println("^");
      ui.nextFrame();
    }
    if(gesture == APDS9960_LEFT) Serial.println("<");
    if(gesture == APDS9960_RIGHT) Serial.println(">");
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
