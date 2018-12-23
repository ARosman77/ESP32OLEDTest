#include <WiFi.h>
#include <WiFiUDP.h>
#include <Preferences.h>
#include <ESP32WebServer.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <SSD1306.h>
#include <BME280I2C.h>
#include <OLEDDisplayUi.h>
#include <Adafruit_APDS9960.h>
#include "images.h"

// Preferences = NVM storage
Preferences ESP32_NVMSettings;
const char *ESP32_namespace = "ESP32ap";
const char *ESP32_ssid_setting = "ssis";
const char *ESP32_pass_setting = "pass";
const char *ESP32_mode_setting = "mode";

// Working modes
enum ESP32_mode { MODE_AP, MODE_CLIENT } wifiMode = MODE_AP;

// default ssid and password for AP
const char *AP_ssid = "ESP32ap";
const char *AP_pass = "12345678";

// ssid and password for WIFI client
String CL_ssid;
String CL_pass;

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


// web server at address 80
ESP32WebServer webServer(80);

// simple web page with forms
const char *AP_WEB_PAGE_HTML = 
"<!DOCTYPE html>\n"
"<html>\n"
"<body>\n"
"<h1 style=\"color:blue;margin-left:30px;\">\n"
"\tESP32 WiFi Settings\n"
"</h1>\n"
"<form action=\"/submit\" method=\"post\">\n"
"    <table>\n"
"    <tr>\n"
"    <td><label class=\"label\">Network Name : </label></td>\n"
"    <td><input type = \"text\" name = \"ssid\"/></td>\n"
"    </tr>\n"
"    <tr>\n"
"    <td><label>Password : </label></td>\n"
"    <td><input type = \"text\" name = \"pass\"/></td>\n"
"    </tr>\n"
"    <tr>\n"
"    <td align=\"center\" colspan=\"2\"><input style=\"color:blue;margin-left:auto;margin-right:auto;\" type=\"submit\" value=\"Submit\"></td>\n"
"    </tr>\n"
"    </table>\n"
"</form>\n"
"</body>\n"
"</html>";

// web server callbacks
void handleAPRoot()
{
  webServer.send(200,"text/html",AP_WEB_PAGE_HTML);
}

void handleAPOnSubmit()
{
  webServer.send(200, "text/plain", "Form submited! Restarting...");
  Serial.print("Form submitted with: ");
  Serial.print(webServer.arg("ssid"));
  Serial.print(" and ");
  Serial.println(webServer.arg("pass"));
  ESP32_NVMSettings.putString(ESP32_ssid_setting,webServer.arg("ssid"));
  ESP32_NVMSettings.putString(ESP32_pass_setting,webServer.arg("pass"));
  ESP32_NVMSettings.putUChar(ESP32_mode_setting,MODE_CLIENT);
  ESP.restart();
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  webServer.send(404, "text/plain", message);
}

unsigned long startTime;
long rssiLevel;
int nFuelLevel;
bool bBurn;

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

void drawTime(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  String output;
  output = "23/12  18:20";
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, output);
}

void drawWiFiLevel(OLEDDisplay *display, OLEDDisplayUiState* state)
{
  // RSSI levels
  /*
      -30 dBm   Amazing
      -67 dBm   Very Good
      -70 dBm   Okay
      -80 dBm   Not Good
      -90 dBm   Unusable
  */
  if (rssiLevel > -67) display->drawXbm(35, 0, WiFi100_width, WiFi100_height, WiFi100_bits);
  else if (rssiLevel > -70) display->drawXbm(35, 0, WiFi50_width, WiFi50_height, WiFi50_bits);
  else if (rssiLevel > -80) display->drawXbm(35, 0, WiFi20_width, WiFi20_height, WiFi20_bits);
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
OverlayCallback overlays[] = { drawLeftLogo, drawTime, drawWiFiLevel };
int overlaysCount = 3;

void setup()
{
  Serial.begin(115200);

  // initialize preferences
  ESP32_NVMSettings.begin(ESP32_namespace, false);
  CL_ssid = ESP32_NVMSettings.getString(ESP32_ssid_setting,"");
  CL_pass = ESP32_NVMSettings.getString(ESP32_pass_setting,"");
  wifiMode = (ESP32_mode)ESP32_NVMSettings.getUChar(ESP32_mode_setting,MODE_AP);

  switch (wifiMode)
  {
    case MODE_CLIENT:
      Serial.println("MODE_CLIENT");
      setup_CL();
      break;
    default:
    case MODE_AP:
      Serial.println("MODE_AP");
      setup_AP();
      break;
  }

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
  rssiLevel = -100;
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

// start in AP mode
void setup_AP()
{
  // start AP
  Serial.println("Configuring access point...");
  WiFi.softAP(AP_ssid, AP_pass);
  IPAddress AP_IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(AP_IP);

  // register web server callback functions
  webServer.on("/",handleAPRoot);
  webServer.onNotFound(handleNotFound);
  webServer.on("/submit",HTTP_POST,handleAPOnSubmit);

  // start web server
  webServer.begin();
  Serial.println("HTTP server started...");
}

// start in client mode
void setup_CL()
{
  // try connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(CL_ssid.c_str());
  
  int nTimeOutCnt = 0;
  WiFi.begin(CL_ssid.c_str(), CL_pass.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    nTimeOutCnt++;
    if(nTimeOutCnt>120)
    {
      Serial.println("Can not connect to AP ...");
      Serial.println("Switching to AP mode for settings ...");
      ESP32_NVMSettings.putUChar(ESP32_mode_setting,MODE_AP);
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // indicate WiFi connection
  rssiLevel = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssiLevel);
  
}

void loop()
{
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0)
  {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    // Serial.print("Remaining Time Budget = ");
    // Serial.print(remainingTimeBudget);
    // Serial.println(" ms");
    webServer.handleClient();
    //read a gesture from the device
    uint8_t gesture = apds.readGesture();
    if(gesture == APDS9960_DOWN) 
    {
      ui.previousFrame();
    }
    if(gesture == APDS9960_UP)
    {
      ui.nextFrame();
    }
    if(gesture == APDS9960_LEFT) Serial.println("<");
    if(gesture == APDS9960_RIGHT) Serial.println(">");
    if ((millis()-startTime)>1000)
    {
      startTime=millis();
      rssiLevel = WiFi.RSSI();
    }
  }
}
