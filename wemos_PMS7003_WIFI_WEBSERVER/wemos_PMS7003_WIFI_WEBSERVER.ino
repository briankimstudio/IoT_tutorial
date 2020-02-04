/**
 * Air quality monitoring system using PMS7003 sensor with WIFI, WEBSERVER
 *           
 * Hardware : Wemos D1 mini, PMS7003
 * Software : Arduino IDE, EPS8266 Sketch Data Upload
 * Library  : ESPAsyncTCP, ESPAsyncWebServer
 * 
 * January 2020. Brian Kim
 */
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

/**
 * WiFi credentials
 */
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"

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

AsyncWebServer server(80);

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

String readSensor() {
  int checksum = 0;
  unsigned char pms[32] = {0,};
  String ret;

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
      ret = String(_pm1) + " " + String(_pm25) + " " + String(_pm10);
      return ret;
    }
    if( pms[0]!=0x42 || pms[1]!=0x4d ) {
      Serial.println("Packet error");
      ret = String(_pm1) + " " + String(_pm25) + " " + String(_pm10);
      return ret;
    }
    _pm1  = makeWord(pms[10],pms[11]);
    _pm25 = makeWord(pms[12],pms[13]);
    _pm10 = makeWord(pms[14],pms[15]);
    ret = String(_pm1) + " " + String(_pm25) + " " + String(_pm10);
    return ret;
  }		
}

void setup()
{
  Serial.begin(115200); // For debugging
  _serial.begin(9600);  // For communicating with PMS7003 sensor
  Serial.printf("\nAir quality monitoring system using PMS7003 sensor with WIFI, WEBSERVER\n");

  // Initialize filesystem
  if(!SPIFFS.begin()){
    Serial.println("ERROR SPIFFS failed");
    return;
  }

  connectWifi(); // Connect to WiFi

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // request->send(SPIFFS, "/index.html", String(), false, templateHandler);
    request->send(SPIFFS, "/index.html", "text/html");
  });  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  /**
   * Send measured data(PM1.0, PM2.5, PM10.0) back to client
   * Data format : Each value is separated by white space
   * Example     : 11 22 33
   */
  server.on("/updatesensorreading", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSensor().c_str()); 
  });
  server.begin();
}

void loop()
{
}