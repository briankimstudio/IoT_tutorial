/**
 * CO2 air quality monitoring system using MH_Z19B sensor with WIFI, WEBSERVER, LINE NOTIFY
 *           
 * Hardware : Wemos D1 mini, MH_Z19B
 * Software : Arduino IDE, EPS8266 Sketch Data Upload
 * Library  : ESPAsyncTCP, ESPAsyncWebServer
 * 
 * Cautious!
 *   After cold start, it takes about 65 seconds for MH_Z19B to preheat itself. During this period,
 *   the reading will be somewhere between 410 and 430. But, after preheating period, it starts showing
 *   actual CO2 density. Therefore, it is recommended to read sensor after 65 seconds at least.
 *   
 * MH-Z19B Manual:
 *   https://www.winsen-sensor.com/d/files/infrared-gas-sensor/mh-z19b-co2-ver1_0.pdf
 * LINE API Reference : 
 *   https://engineering.linecorp.com/en/blog/using-line-notify-to-send-messages-to-line-from-the-command-line/
 * 
 * January 2020. Brian Kim
 */
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <WiFiClientSecureAxTLS.h>

/**
 * WiFi credentials
 */
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"

/**
 * LINE Notify credential
 */
#define LINE_TOKEN "your-line-token"

/**
 * MH_Z19B sensor pin map and command packet
 */
#define MH_Z19B_TX          D5   // GPIO12
#define MH_Z19B_RX          D6   // GPIO14
#define MH_Z19B_CMD         0xFF
#define MH_Z19B_RES         0x86
#define MH_Z19B_DATA_LENGTH 9
byte _cmd[MH_Z19B_DATA_LENGTH] = {MH_Z19B_CMD,0x01,MH_Z19B_RES,0x00,0x00,0x00,0x00,0x00,0x79};

/**  
 *   Wemos serial RX - TX MH_Z19B
 *                TX - RX
 */
SoftwareSerial _serial(MH_Z19B_TX, MH_Z19B_RX); // RX, TX
AsyncWebServer server(80);

int _co2;

unsigned long previousMillis = 0;
const long notifyInterval    = 600000; // Waiting time(milliseconds)
const int  notifyLevel       = 1000; // CO2 threshold for notification
bool       waitFlag          = false;

/**
 * Preheating timing
 */
unsigned long startupMillis = 0;

/**
 * Measuring interval
 */
unsigned long measuringMillis = 0;
const long measuringInterval  = 5000; // Waiting time(milliseconds)

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

void notifyLine(String msg) {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("notify-api.line.me", 443)) {
    Serial.println ("ERROR Connection failed");
    return;
  }
  
  String req = "POST /api/notify HTTP/1.1\r\n";
  req += "Host: notify-api.line.me\r\n";
  req += "Authorization: Bearer " + String(LINE_TOKEN) + "\r\n";
  req += "Cache-Control: no-cache\r\n";
  req += "User-Agent: ESP8266\r\n";
  req += "Connection: close\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(String("message=" + msg).length()) + "\r\n";
  req += "\r\n";
  req += "message=" + msg;
  client.print(req);
  Serial.print(req);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
//        delay(20000);
      break;
    }
    Serial.println(line);
  }
}

String readSensor() {
  byte checksum = 0;
  unsigned char res[MH_Z19B_DATA_LENGTH] = {0,};
  int avail = 0;
  int count = 0;
  _serial.flush();
  _serial.write(_cmd, MH_Z19B_DATA_LENGTH);
  delay(400);
  while(_serial.available() &&
        _serial.read() != MH_Z19B_CMD &&
        _serial.peek() != MH_Z19B_RES ) {
  }
  if ( _serial.available() >= MH_Z19B_DATA_LENGTH-1 ) {
    res[0] = MH_Z19B_CMD;
    for (int j=1;j<MH_Z19B_DATA_LENGTH; j++) {
      res[j] = _serial.read();
      // Serial.printf("%2X ",res[j]);
      if ( j < MH_Z19B_DATA_LENGTH-1 )
        checksum += res[j];
    }
    // Serial.println();
    checksum = 0xFF - checksum;
    checksum++;
    if (res[8] == checksum) {
      String status;
      unsigned long uptime = (millis()-startupMillis)/1000;
      _co2 = makeWord(res[2],res[3]);

      if ( _co2 <= 430 && uptime < 65000 ) {
        status = "Preheating";
      } else {
        status = "Measuring";
      }

      Serial.printf("CO2:%4d TEMP:%2d ELAPSED:%d Seconds STATUS:%s\n", _co2,
                                                             res[4]-44,
                                                             uptime,
                                                             status.c_str());
    } else {
      Serial.printf("Error: CMD:%X RES:%X Checksum:%X:%X\n", res[0], res[1], checksum, res[8]);
    }
  } else {
    Serial.println("Not ready");
  }
  return String(_co2);
}

void setup()
{
  startupMillis = millis();

  Serial.begin(115200); // For debugging
  _serial.begin(9600);  // For communicating with MH_Z19B sensor
  Serial.printf("\nCO2 air quality monitoring system using MH-Z19B sensor with WIFI, WEBSERVER, LINE NOTIFY\n");
  Serial.printf("CO2 notify level:%d Waiting time:%d seconds\n",notifyLevel, notifyInterval/1000); 

  // Initialize filesystem
  if(!SPIFFS.begin()){
    Serial.println("ERROR SPIFFS failed");
    return;
  }

  connectWifi(); // Connect to WiFi
  String msg="C02 sensor http://"+WiFi.localIP().toString();
  // notifyLine(msg);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // request->send(SPIFFS, "/index.html", String(), false, templateHandler);
    request->send(SPIFFS, "/index.html", "text/html");
  });  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/line_logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/line_logo.png", "image/png");
  });
  
  /**
   * Send measured data back to client
   * Data format : CO2
   * Example     : 500
   */
  server.on("/updatesensorreading", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(_co2).c_str());     
  });
  server.begin();
}

void loop()
{
  unsigned long currentMillis = 0;
  unsigned long currentMeasuringMillis = 0;

  if (_co2 >= notifyLevel && !waitFlag) {
    String msg = "CO2 level is  ";
    Serial.printf("CO2: %d\n",_co2);
    msg += String(_co2);
    // notifyLine(msg);
    waitFlag = true;
    previousMillis = millis();
  }
  if (waitFlag) {
    currentMillis = millis();
    if ( currentMillis - previousMillis >= notifyInterval ) {
      waitFlag = false;
      previousMillis = currentMillis;
    }
  }

  currentMeasuringMillis = millis();
  if ( currentMeasuringMillis - measuringMillis > measuringInterval ) {
    readSensor();
    measuringMillis = currentMeasuringMillis;
  }
}