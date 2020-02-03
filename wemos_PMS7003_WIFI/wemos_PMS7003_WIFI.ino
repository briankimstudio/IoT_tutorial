#include <SoftwareSerial.h>

/**
 * PMS7003 sensor pin map and packet header
 */
#define PMS7003_TX D5 // GPIO14
#define PMS7003_RX D6 // GPIO12
#define PMS7003_PREAMBLE_1 0x42
#define PMS7003_PREAMBLE_2 0x4D
#define PMS7003_DATA_LENGTH 31

/**
 * thingspeak credentials
 */
#define TS_URL
#define TS_CHANNEL
#define TS_KEY

SoftwareSerial _serial(PMS7003_RX, PMS7003_TX); // RX, TX

int _pm1, _pm25, _pm10;

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
      // Serial.print(pms[j],HEX);Serial.print(" ");
      if(j < 30)
        checksum += pms[j];
    }
    // Serial.println(" ");
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

void sendToCloud() {
  char datetimeStr[25];
  String postStr = "write_api_key=" + TS_KEY +"&time_format=absolute&updates=";
  sprintf(datetimeStr,"%4d-%02d-%02dT%02d:%02d:%02d",year(),month(),day(),hour(),minute(),second());
  // 2018-06-14T12:12:22-0500
  postStr += String(datetimeStr)+"+0800" 
              + "," + String(_pm1)
              + "," + String(_pm25)
              + "," + String(_pm10);

  if ( !client ) { return false; }

  if (client->connect( _cloudUrl , 80 )) {
    client->println( "POST /channels/"+_cloudChannel+"/bulk_update.csv HTTP/1.1" );
    client->println( "Host: api.thingspeak.com" );
    client->println( "Connection: close" );
    client->println( "Content-Type: application/x-www-form-urlencoded" );
    client->println( "Content-Length: " + String( postStr.length() ) );
    client->println();
    client->println( postStr );
    VERBOSELN( postStr );
    String answer=getResponse();
    if ( !answer.indexOf("202 Accepted") ) {
      VERBOSELN("NM : sync : ERROR POST failed");
    } else {
      VERBOSELN("NM : sync : OK");
    }
  } else {
    VERBOSELN ( "NM : sync : ERROR Connection failed" );  
    return false;
  }
}

void setup()
{
  Serial.begin(115200); // For debugging
  _serial.begin(9600);  // For communicating with PMS7003 sensor
}

void loop()
{
  readSensor();
  Serial.printf("PM1.0 %d, PM2.5 %d PM10.0 %d", _pm1, _pm25, _pm10);
  delay(2000);
}
