# NeoESP8266Clock
ESP8266 based NeoPixel Clock

by Frank Hellmann, Hamburg, Germany, March 2016

frank@vfx.to

www.vfx.to

Idea from a www.ESP8266.com post by Schufti http://www.esp8266.com/viewtopic.php?f=29&t=5886#p35743

Tested with:
Arduino 1.8.3 IDE

NodeMCU 1.0 Board (ESP12-E)

ESP8266 Version 2.3.0 (Board Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json)

WiFiManager 0.10.0 

History:
--------

v0.2 - 16. March 2016 - WiFiManager autoconfiguration

v0.5 - 03. April 2016 - added Documentation and Daylight Savings Adjust autoconfiguration

v0.6 - 22. Feb.  2017 - added DNS lookup for NTP Timeserver

v0.7 - 25. March 2018 - rewrite for new NTP and simpleDSTadjust lib

The hardware is pretty simple to build. You'll need:

1 * NodeMCU 1.0 board (or a similar ESP8266 based board)

4 * Adafruit NeoPixel 1/4 Ring 60 WS2812  

1 * 5V 1A Micro-USB power supply 

Solder a complete 60 LED Ring out of the 1/4 parts and connect it to the NodeMCU board:
- Leave one DOUT unconnected to the next DIN and scrape some of the solder pad away to make sure there is no connection
- This is the end of the ring and the DIN pad next to it is the beginning of the ring 
- Connect the other DOUT pads to the DIN pads next to them 
- Connect all +5V powersupply pads
- Connect all GND powersupply pads
- Connect the +5V pad to the NodeMCU VUSB Pin
- Connect the DIN pad to the NodeMCU D2 Pin   (see the #define below)
- Connect the GND pad to one of the NodeMCU GND Pins 

That's it! Now upload the sketch und connect your laptop to the ESP8266Clock Access Point. 
Now open a browser and surf to any address. The ESP8266Clock will redirect to it's configuration page. 

Have fun and see you at attrakor makerspace hamburg, www.attraktor.org !
