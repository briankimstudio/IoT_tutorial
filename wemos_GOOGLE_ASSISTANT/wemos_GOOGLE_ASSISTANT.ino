/**
 * Controlling ESP8266 with Google Assistant 
 *           
 * Hardware : Wemos D1 mini, LED strip
 * Software : Arduino IDE, Adafruit MQTT Library, Adafruit NeoPixel Library
 *
 * LED control functions are from
 *   https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/RGBWstrandtest/RGBWstrandtest.ino
 * 
 * January 2020. Brian Kim
 */

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_NeoPixel.h>

/**
 * WiFi credentials
 */
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"

/**
 * MQTT broker credentials
 */
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "...your AIO username (see https://accounts.adafruit.com)..."
#define AIO_KEY         "...your AIO key..."

/**
 * From Adafruit NeoPixel example 
 */
#define LED_PIN    D1
#define LED_COUNT  8
#define BRIGHTNESS 50
Adafruit_NeoPixel  strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

void setup() {
  Serial.begin(115200); // For debugging
  Serial.printf("\nControlling ESP8266 with Google Assistant via IFTTT, MQTT\n");
  connectWifi();        // Connect to WiFi

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  // strip.show();            // Turn OFF all pixels ASAP
  turnOff();
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  mqtt.subscribe(&onoffbutton);
}

void loop() {
  connectMQTT();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Received command: "));
      Serial.println((char *)onoffbutton.lastread);
      String command = String((char *)onoffbutton.lastread);
      if ( command.equals("on") ) {
        Serial.println("Turn on the light");
        turnOn();
      } else if (command.equals("off")) {
        Serial.println("Turn off the light");
        turnOff();
      }
    }
  }

  // if(!mqtt.ping()) {
  //   mqtt.disconnect();
  // }  
}

void turnOff() {
  strip.clear();
  strip.show();            // Turn OFF all pixels ASAP
}

void turnOn() {
  colorWipe(strip.Color(255,   0,   0)     , 50); // Red
  colorWipe(strip.Color(  0, 255,   0)     , 50); // Green
  colorWipe(strip.Color(  0,   0, 255)     , 50); // Blue
  colorWipe(strip.Color(  0,   0,   0, 255), 50); // True white (not RGB white)
  // whiteOverRainbow(75, 5);
  // pulseWhite(5);
  // rainbowFade2White(3, 3, 1);
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

/**
 * From https://github.com/adafruit/Adafruit_MQTT_Library/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
 */
void connectMQTT() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}