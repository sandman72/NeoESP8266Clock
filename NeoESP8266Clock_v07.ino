//---------------------------------------------------
//             NeoESP8266Clock
//---------------------------------------------------
//
// by Frank Hellmann, Hamburg, Germany, March 2016
// frank@vfx.to
// www.vfx.to
//
// Idea from a www.ESP8266.com post by Schufti http://www.esp8266.com/viewtopic.php?f=29&t=5886#p35743
//
// Tested with:
// Arduino 1.8.3 IDE
// NodeMCU 1.0 Board (ESP12-E)
// ESP8266 Version 2.3.0 (Board Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json)
// WiFiManager 0.10.0 
//
// History:
// --------
// v0.2 - 16. March 2016 - WiFiManager autoconfiguration
// v0.5 - 03. April 2016 - added Documentation and Daylight Savings Adjust autoconfiguration
// v0.6 - 22. Feb.  2017 - added DNS lookup for NTP Timeserver
// v0.7 - 25. March 2018 - rewrite for new NTP and simpleDSTadjust lib
//
// The hardware is pretty simple to build. You'll need:
//
// 1 * NodeMCU 1.0 board (or a similar ESP8266 based board)
// 4 * Adafruit NeoPixel 1/4 Ring 60 WS2812  
// 1 * 5V 1A Micro-USB power supply 
//
// Solder a complete 60 LED Ring out of the 1/4 parts and connect it to the NodeMCU board:
// - Leave one DOUT unconnected to the next DIN and scrape some of the solder pad away to make sure there is no connection
// - This is the end of the ring and the DIN pad next to it is the beginning of the ring 
// - Connect the other DOUT pads to the DIN pads next to them 
// - Connect all +5V powersupply pads
// - Connect all GND powersupply pads
// - Connect the +5V pad to the NodeMCU VUSB Pin
// - Connect the DIN pad to the NodeMCU D2 Pin   (see the #define below)
// - Connect the GND pad to one of the NodeMCU GND Pins 
//
// That's it! Now upload the sketch und connect your laptop to the ESP8266Clock Access Point. 
// Now open a browser and surf to any address. The ESP8266Clock will redirect to it's configuration page. 
//
// Have fun and see you at attrakor makerspace hamburg, www.attraktor.org !
//

// Libraries used                    Name                       Comes with
#include <Adafruit_NeoPixel.h>    // Adafruit NeoPixel Library (Lib Manager)
#include <ESP8266WiFi.h>          // ESP8266 WiFi Library      (ESP8266 Board Install)
//#include <ESP8266WebServer.h>     // ESP8266 Webserver Library (ESP8266 Board Install)
//#include <WiFiUdp.h>              // ESP8266 UDP Library       (ESP8266 Board Install)
//#include <DNSServer.h>            // ESP8266 DNS Library       (ESP8266 Board Install)
#include <WiFiManager.h>          // Tazpu's WiFiManager       (Lib Manager)
#include <Ticker.h>               // ESP8266 Ticker Library    (ESP8266 Board Install)
#include <time.h>                 // ESP8266 time Library      (ESP8266 Board Install)
#include <simpleDSTadjust.h>      // simpleDSTadjust Library   (Lib Manager)

// Hardware Pins
#define PIN D2                    // Pin for NeoPixel Ring Data Output 
#define BTN 0                     // Pin for Button
 
// My LED Ring starts on the 6 o'clock position, so the LED Position needs to move around 30 Pixels
#define LED_OFFSET 30

// Update time from NTP server every 3 hours
#define NTP_UPDATE_INTERVAL_SEC 3*3600

// Maximum of 3 servers
#define NTP_SERVERS "de.pool.ntp.org", "pool.ntp.org", "time.nist.gov"

// Daylight Savings Adjust
#define timezone +1
char *dstAbbrev;
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour
// Example Settings for Boston
// #define UTC_OFFSET -5
// struct dstRule StartRule = {"EDT", Second, Sun, Mar, 2, 3600}; // Eastern Daylight time = UTC/GMT -4 hours
// struct dstRule EndRule = {"EST", First, Sun, Nov, 1, 0};  

int hz, mz, sz, hc;      // Pixel Positions on the LED Ring for hour, minute, second
int cc;

Ticker ticker1;
int32_t tick;

// flag changed in the ticker function to start NTP Update
bool readyForNtpUpdate = false;

// Setup simpleDSTadjust Library rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

// Setup NeoPixel
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);
#define NIGHTMODE 0x1F                    // adjust brightness between 23 and 8 o'clock
#define FULLMODE  0xFF
#define HUE 60                            // Hue Divider (1 round = 60 LEDs)


//----------------------- Functions -------------------------------

// NTP timer update ticker
void secTicker() {
  tick--;
  if(tick<=0)
   {
    readyForNtpUpdate = true;
    tick= NTP_UPDATE_INTERVAL_SEC; // Re-arm
   }
  // printTime(0);  // Uncomment if you want to see time printed every second
}

// Update time via NTP
void updateNTP() {
  configTime(timezone * 3600, 0, NTP_SERVERS);
  delay(500);
  while (!time(nullptr)) {
    Serial.print("#");
    delay(1000);
  }
}

// Print time to serial
void printTime(time_t offset){
  char buf[30];
  time_t t = dstAdjusted.time(&dstAbbrev)+offset;
  struct tm *timeinfo = localtime (&t);
  
  int hour = (timeinfo->tm_hour+11)%12+1;  // take care of noon and midnight
  sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d%s %s\n",timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_year+1900, hour, timeinfo->tm_min, timeinfo->tm_sec, timeinfo->tm_hour>=12?"pm":"am", dstAbbrev);
  Serial.print(buf);
}

// Setup WiFi
// gets called when WiFiManager enters configuration mode and servers webpage for configuration
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFi Manager config mode");
  Serial.println(WiFi.softAPIP());
}

// utility to convert hue, saturation and intensity to RGB values 
unsigned long hsi_rgb(float H, float S, float I) {
  unsigned int r, g, b;
  H = 3.14159 * H / (float)180;    // convert H to radians.
  S = S > 0 ? (S < 1 ? S : 1) : 0; // clamp S and I to interval [0,1]
  I = I > 0 ? (I < 1 ? I : 1) : 0;

  // Math! Thanks in part to Kyle Miller.
  if (H < 2.09439) {
    r = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    g = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    b = 255 * I / 3 * (1 - S);
  } else if (H < 4.188787) {
    H = H - 2.09439;
    g = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    b = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    r = 255 * I / 3 * (1 - S);
  } else {
    H = H - 4.188787;
    b = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    r = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    g = 255 * I / 3 * (1 - S);
  }
  return (long(r) << 16) | (g << 8) | (b & 0xFF);
}

void setup() {
  // lets settle for a moment
  delay(100);

  // init serial
  Serial.begin(115200);
  Serial.println();

  // init Neopixels
  strip.begin();
  strip.clear();
  strip.show();
  
  // show rainbow   -> Term (i +30) % 60 is for 6 o'clock LED rings
  for (byte i = 0; i < 60; i++)
    strip.setPixelColor((i + LED_OFFSET) % 60, hsi_rgb((360 / HUE) * (i % HUE), 0.93, 0.1));
  strip.show();

  // Check for WiFiManager Configuration
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  wifiManager.setConfigPortalTimeout(60);
  //and goes into a blocking loop awaiting configuration for 60secs
  if (!wifiManager.autoConnect("NeoESP8266Clock")) {
    Serial.println("failed to connect and hit timeout");
  } else {
    //if you get here you have connected to the WiFi
    Serial.println("WiFi connected...yay :)");
    Serial.print("IP number assigned by DHCP is ");
    Serial.println(WiFi.localIP());

    Serial.print("Update time via NTP: ");
    updateNTP(); // Init the NTP time
    printTime(0); // print initial time time now.
    tick = NTP_UPDATE_INTERVAL_SEC; // Init the NTP update countdown ticker
    ticker1.attach(1, secTicker); // Run a 1 second interval Ticker
    Serial.print("Next NTP Update: ");
    printTime(tick);
  }

  // wait for initial NTP Package and check for Daylight Savings Time
  delay(2000);
}

void loop() {
   // Check if we need to update time via NTP
   if(readyForNtpUpdate) {
    readyForNtpUpdate = false;
    printTime(0);
    updateNTP();
    Serial.print("\nUpdated time from NTP Server: ");
    printTime(0);
    Serial.print("Next NTP Update: ");
    printTime(tick);
  }

  time_t t = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime (&t);
  
  hz = timeinfo->tm_hour;
  mz = timeinfo->tm_min;
  sz = timeinfo->tm_sec;

  // Check for nightmode brightness and Daylight Savings Time
  if (hz != hc) {
    if (hz < 8) {
      strip.setBrightness(NIGHTMODE);
    } else {
      strip.setBrightness(FULLMODE);
    }
    hc = hz;
  }

  // Do display update
  // You can change the update speed for the pixels and the background colors here:
  if (millis() % 125 > 110) { 
    strip.clear();

    // fill LED Ring with BG Color  -> Term (xx +30) % 60 is for 6 o'clock LED rings
    for (byte i = 0; i < 60; i++)
      strip.setPixelColor((i + LED_OFFSET) % 60, hsi_rgb((360 / HUE) * ((i + cc) % HUE), 0.93, 0.1));

    cc = (cc + 1) % HUE;    // Move background color one pixel counter clockwise

    // Hours, Minutes and Second Pixels
    // Mixing the colors in this tree if positions are identical:

    hz = int(hz * 2.5) + int(mz / 20);

    // Seconds
    strip.setPixelColor((sz + LED_OFFSET) % 60, 0x7F, 0x00, 0x00);        //  S       -> Red

    // Minutes
    if (mz == sz) {
      strip.setPixelColor((mz + LED_OFFSET) % 60, 0x7F, 0x5F, 0x00);      //  M = S   -> Yellow
    } else {
      strip.setPixelColor((mz + LED_OFFSET) % 60, 0x00, 0x5F, 0x00);      //  M       -> Green 
    }
    
    // Hours
    if (hz == sz) {
      if (hz == mz) {
        strip.setPixelColor((hz + LED_OFFSET) % 60, 0x7F, 0x5F, 0xAF);     //  H=M=S -> White
      } else {
        strip.setPixelColor((hz + LED_OFFSET) % 60, 0x7F, 0x00, 0xAF);     //  H = S -> Pink
      }
    } else {
      if (hz == mz) {
        strip.setPixelColor((hz + LED_OFFSET) % 60, 0x00, 0x5F, 0xAF);     //  H = M -> Cyan
      } else {
        strip.setPixelColor((hz + LED_OFFSET) % 60, 0x00, 0x00, 0xAF);     //  H     -> Blue
      }
    }

    strip.show();
    delay(10);
  }
}
