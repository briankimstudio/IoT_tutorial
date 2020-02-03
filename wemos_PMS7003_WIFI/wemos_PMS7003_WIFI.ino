/**
 * Air quality monitoring system using PMS7003 sensor with WIFI
 *           
 * Hardware : Wemos D1 mini, PMS7003
 * Software : Arduino IDE
 * 
 * January 2020. Brian Kim
 */
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>

/**
 * WiFi credentials
 */
#define WIFI_SSID "your-ssid"
#define WIFI_PASS "your-password"

/**
 * Thingspeak credentials
 */
#define TS_URL    "api.thingspeak.com"
#define TS_KEY    "your-ts-channel-key"

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

void uploadCloud(int pm1, int pm25, int pm10) {
  WiFiClient client;
  char datetime_str[25];
  // Format : https://api.thingspeak.com/update.json?api_key=<write_api_key>&field1=123
  String getStr = "GET /update.json?api_key=" + String(TS_KEY) 
                + "&field1=" + String(pm1)
                + "&field2=" + String(pm25)
                + "&field3=" + String(pm10) 
                + " HTTP/1.0";
  Serial.println( getStr );

  if (client.connect(TS_URL, 80)) {
    client.println( getStr );
    client.println();
  } else {
    Serial.println ("ERROR Connection failed"); 
  }
  // Wait for response from thingspeak
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("ERROR Client timeout");
      client.stop();
      delay(60000);
      return;
    }
  }

  Serial.println("Response from thingspeak :");
  while (client.available()) {
    char ch = static_cast<char>(client.read());
    Serial.print(ch);
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
  Serial.printf("\nAir quality monitoring system using PMS7003 sensor with WIFI\n");
  connectWifi();        // Connect to WiFi
}

void loop()
{
  readSensor();
  Serial.printf("PM1.0:%d PM2.5:%d PM10.0:%d\n", _pm1, _pm25, _pm10);
  uploadCloud(_pm1, _pm25, _pm10); // Upload to thingspeak cloud
  delay(300000); // Wait 5 minutes
}
