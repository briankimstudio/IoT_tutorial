/**
 * Air quality monitoring system using PMS7003 sensor with WIFI, MQTT
 *           
 * Hardware : Wemos D1 mini, PMS7003
 * Software : Arduino IDE, Adafruit MQTT Library
 * 
 * January 2020. Brian Kim
 */
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

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
 * PMS7003 sensor pin map and packet header
 */
#define PMS7003_TX          D5   // GPIO12
#define PMS7003_RX          D6   // GPIO14
#define PMS7003_PREAMBLE_1  0x42 // From PMS7003 datasheet
#define PMS7003_PREAMBLE_2  0x4D
#define PMS7003_DATA_LENGTH 31

/**  
 *   Wemos serial RX - TX PMS7003
 *                TX - RX
 */
SoftwareSerial _serial(PMS7003_TX, PMS7003_RX); // RX, TX

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Create feeds for MQTT
Adafruit_MQTT_Publish pm1Feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm1");
Adafruit_MQTT_Publish pm25Feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm2-dot-5");
Adafruit_MQTT_Publish pm10Feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pm10");

int _pm1, _pm25, _pm10;

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

void publishMQTT(int pm1, int pm25, int pm10) {
  if (!pm1Feed.publish(pm1)) {
    Serial.println("pm1Feed failed");
  } else {
    Serial.println("pm1Feed Ok");
  }
  if (!pm25Feed.publish(pm25)) {
    Serial.println("pm25Feed failed");
  } else {
    Serial.println("pm25Feed Ok");
  }
  if (!pm10Feed.publish(pm10)) {
    Serial.println("pm10Feed failed");
  } else {
    Serial.println("pm10Feed Ok");
  }
}

void readSensor() {
  int checksum = 0;
  unsigned char pms[32] = {0,};

  /**
   * Search preamble for Packet
   * Solve trouble caused by delay function
   */
  while( _serial.available() && 
      _serial.read() != PMS7003_PREAMBLE_1 &&
      _serial.peek() != PMS7003_PREAMBLE_2 ) {
  }		
  if( _serial.available() >= PMS7003_DATA_LENGTH ){
    pms[0] = PMS7003_PREAMBLE_1;
    checksum += pms[0];
    for(int j=1; j<32 ; j++){
      pms[j] = _serial.read();
      if(j < 30)
        checksum += pms[j];
    }
    _serial.flush();
    if( pms[30] != (unsigned char)(checksum>>8) 
      || pms[31]!= (unsigned char)(checksum) ){
      Serial.println("Checksum error");
      return;
    }
    if( pms[0]!=0x42 || pms[1]!=0x4d ) {
      Serial.println("Packet error");
      return;
    }
    _pm1  = makeWord(pms[10],pms[11]);
    _pm25 = makeWord(pms[12],pms[13]);
    _pm10 = makeWord(pms[14],pms[15]);
  }		
}

void setup()
{
  Serial.begin(115200); // For debugging
  _serial.begin(9600);  // For communicating with PMS7003 sensor
  Serial.printf("\nAir quality monitoring system using PMS7003 sensor with WIFI, MQTT\n");
  connectWifi();        // Connect to WiFi
  delay(2000);
}

void loop()
{
  readSensor();
  Serial.printf("PM1.0:%d PM2.5:%d PM10.0:%d\n", _pm1, _pm25, _pm10);
  connectMQTT();
  publishMQTT(_pm1, _pm25, _pm10); // Publish to MQTT broker
  delay(300000); // Wait 5 minutes
}
