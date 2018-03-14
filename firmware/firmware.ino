#include <Arduino.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <SerialCommands.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#define PIN_READ( pin )  GPIP(pin)
#define PINC_READ( pin ) digitalRead(pin)



#define MODEPIN_SNES 0
#define MODEPIN_N64  1
#define MODEPIN_GC   13
#define N64_PIN        5
#define N64_PREFIX     8
#define N64_BITCOUNT  32

#define SNES_LATCH      3
#define SNES_DATA       4
#define SNES_CLOCK      6
#define SNES_BITCOUNT  16
#define NES_BITCOUNT    8

#define GC_PIN        5
#define GC_PREFIX    24
#define GC_BITCOUNT  64

#define ZERO  '0'  // Use a byte value of 0x00 to represent a bit with value 0.
#define ONE    '1'  // Use an ASCII one to represent a bit with value 1.  This makes Arduino debugging easier.
#define SPLIT '\n'  // Use a new-line character to split up the controller state packets.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NintendoSpy Firmware for Arduino
// v1.0.1
// Written by jaburns
// Heavily modified for ESP8266 by Geekboy1011


// ---------- Uncomment one of these options to select operation mode --------------
//#define MODE_GC
//#define MODE_N64
//#define MODE_SNES
//#define MODE_NES
// Bridge one of the analog GND to the right analog IN to enable your selected mode
#define MODE_DETECT
// ---------------------------------------------------------------------------------
char use_ssid[32] = "";
char use_password[32] = "";

#define WAIT_FALLING_EDGE( pin,timeoutus )  while( !PIN_READ(pin) ); unsigned long sMicros = micros(); while( PIN_READ(pin) ) { if ((micros() - sMicros) > timeoutus){  memset(rawData,0,128); return;}}
#define HEARTBEATMSG "pong"
// Declare some space to store the bits we read from a controller.
unsigned char rawData[ 128 ];
char rawData2[ 128 ];
volatile unsigned char defined_bits=1;
bool trans_pending = false;
bool use_serial = false;
unsigned char GC_PREFIX_STRING[24] = {1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1};
unsigned char N64_PREFIX_STRING[8] = {0,0,0,0,0,0,1,1};

unsigned long heartbeatMillis;
char serial_command_buffer_[256];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\n", " ");

ESP8266WiFiMulti WiFiMulti;
WebSocketsServer webSocket = WebSocketsServer(18881);


//This is the default handler, and gets called when no other command matches.
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
  sender->GetSerial()->print("Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}
void cmd_rwifi(SerialCommands* sender){
  loadCredentials();
  sender->GetSerial()->println(use_ssid);
  sender->GetSerial()->println(use_password);
}
void cmd_wifi(SerialCommands* sender)
{
  char* ssid = sender->Next();
  char* password = sender->Next();
  if (ssid == NULL)
  {
    sender->GetSerial()->println("ERROR SSID");
    return;
  }


  if (password == NULL)
  {
    sender->GetSerial()->println("ERROR PASS");
    return;
  }
  strcpy(use_password, password);
  strcpy ( use_ssid, ssid);
  saveCredentials();
}
void cmd_tserial(SerialCommands* sender){
  char* s = sender->Next();
  if (s==NULL){
     sender->GetSerial()->println("BAD MODE");
     return;
  }
  if (s[0] == '0'){
    use_serial=false;
    sender->GetSerial()->println("WEB SOCKET MODE");
    return;
  }
  if (s[0] == '1'){
    use_serial=true;
    sender->GetSerial()->println("SERIAL MODE");
  }

}
SerialCommand cmd_wifi_write_("WW", cmd_wifi);
SerialCommand cmd_wifi_read_("RW", cmd_rwifi);
SerialCommand cmd_tog_ser_("TS", cmd_tserial);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED://
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);
            if (memcmp(payload,"ping",4) == 0){
                webSocket.sendTXT(num,HEARTBEATMSG);
            }
            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ISR for spying GC and N64 controller commands
// Performs a read cycle from one of Nintendo's one-wire interface based controllers.
// This includes the N64 and the Gamecube.
//     pin  = Pin index on Port D where the data wire is attached.
//     defined_bits = Number of bits to read from the line.
void gc_n64_isr()
{   //Takes ~2us to enter
  GPOS = (1 << 14);
  unsigned char *rawDataPtr = rawData;
  GPOC = (1 << 14);
  do {
    // Wait for the line to go high then low.
    while( !PIN_READ(5) );
    unsigned long sMicros = micros();
    while( PIN_READ(5) ) {
      if ((micros() - sMicros) >= 4){
        memset(rawData,0,sizeof(rawData));
        attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
        return;
        }
    }
    // Wait ~2us between line reads
    GPOS = ( 1 << 14);
    delayMicroseconds(1);
    // Read a bit from the line and store as a byte in "rawData"
    *rawDataPtr = PIN_READ(5);
    GPOC = ( 1 << 14);
    ++rawDataPtr;
  }
  while(--defined_bits != 0 );
  GPOS =( 1 << 14);
  trans_pending = true;
  detachInterrupt(digitalPinToInterrupt(5));  //remove interrupt to not have a requeue before send
  //GPIEC = (1 << 5);  // Clear interrupt flags because we most defineitily triggered one during this interrupt
  GPOC = (1 << 14);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General initialization, just sets all pins to input and starts serial communication.
void setup()
{

   pinMode(N64_PIN,INPUT);
   pinMode(MODEPIN_GC,INPUT);
   pinMode(14,OUTPUT);
   Serial.begin( 115200 );
   serial_commands_.SetDefaultHandler(cmd_unrecognized);
   serial_commands_.AddCommand(&cmd_wifi_read_);
   serial_commands_.AddCommand(&cmd_wifi_write_);
   serial_commands_.AddCommand(&cmd_tog_ser_);
   Serial.println("Connecting");
   WiFi.hostname("NintendoSpy");
   MDNS.addService("http","tcp",18881);
   loadCredentials();
   WiFiMulti.addAP(use_ssid, use_password);

   while(WiFiMulti.run() != WL_CONNECTED) {
        serial_commands_.ReadSerial();
   }
   Serial.print("Connected: ");
   Serial.println(WiFi.localIP());
   webSocket.onEvent(webSocketEvent);
   webSocket.begin();
   ArduinoOTA.setHostname("NintendoSpy");
   ArduinoOTA.begin();


   attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
   heartbeatMillis = millis();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Preferred method for reading SNES + NES controller data.
template< unsigned char latch, unsigned char data, unsigned char clock >
void read_shiftRegister( unsigned char bits )
{
    unsigned char *rawDataPtr = rawData;

    WAIT_FALLING_EDGE( latch,10 );

    do {
        WAIT_FALLING_EDGE( clock,10 );
        *rawDataPtr = !PIN_READ(data);
        ++rawDataPtr;
    }
    while( --bits > 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sends a packet of controller data over the Arduino serial interface.
inline void sendRawData( unsigned char first, unsigned char count )
{
    for( unsigned char i = first ; i < first + count ; i++ ) {
        Serial.write( rawData[i] ? ONE : ZERO );
    }
    Serial.write( SPLIT );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Update loop definitions for the various console modes.

inline void loop_gc()
{
    if (trans_pending){
        if (memcmp(rawData,GC_PREFIX_STRING,sizeof(GC_PREFIX_STRING)) == 0){  //Check that we recieved a packet validly
                for( unsigned char i = GC_PREFIX ; i < GC_PREFIX + GC_BITCOUNT; i++ ) {
                  rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                  if (use_serial){
                    Serial.write(rawData[i]);
                  }
                }
            if (use_serial){
              Serial.write(SPLIT);
            }
            else {
              webSocket.broadcastTXT(rawData+GC_PREFIX,GC_BITCOUNT);
            }

        }
        else {
              for( unsigned char i = 0 ; i < GC_PREFIX + GC_BITCOUNT; i++ ) {
                    rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                    //Serial.write(rawData[i]);
               }
               //Serial.print("Bad GCN Frame: "); Serial.printf("%s",rawData); Serial.println(' ');
            }
            memset(rawData,0,sizeof(rawData)); //Clear frame incase we got bad frame
            trans_pending = false;
            unsigned long startmillis = micros();
            while((micros()-startmillis) < 100 ){
              if(!PIN_READ(5)){
                startmillis = micros();
              }
            }
            defined_bits = GC_PREFIX + GC_BITCOUNT;
            attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
    }
}

inline void loop_N64()
{
    if (trans_pending){
        if (memcmp(rawData,N64_PREFIX_STRING,sizeof(N64_PREFIX_STRING)) == 0){  //Check that we recieved a packet validly
           //Serial.print("Good Frame: ");
                for( unsigned char i = N64_PREFIX ; i < N64_PREFIX +N64_BITCOUNT; i++ ) {
                    rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                    if (use_serial){
                      Serial.write(rawData[i]);
                    }
                }
                if (use_serial){
                  Serial.write(SPLIT);
                }
                else {
                  webSocket.broadcastTXT(rawData+N64_PREFIX,N64_BITCOUNT);
                }
            }
            else {
              for( unsigned char i = 0 ; i < N64_PREFIX +N64_BITCOUNT; i++ ) {
                    rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                    //Serial.write(rawData[i]);
               }
               Serial.print("Bad N64 Frame: "); Serial.printf("%s",rawData); Serial.println(' ');
            }
            memset(rawData,0,sizeof(rawData)); //Clear frame incase we got bad frame
            trans_pending = false;
            unsigned long startmillis = micros();
            while((micros()-startmillis) < 100 ){
              if(!PIN_READ(5)){
                startmillis = micros();
              }
            }
            attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
            defined_bits = N64_PREFIX + N64_BITCOUNT;
    }
}

inline void loop_SNES()
{
    noInterrupts();
    read_shiftRegister< SNES_LATCH , SNES_DATA , SNES_CLOCK >( SNES_BITCOUNT );
    interrupts();
    sendRawData( 0 , SNES_BITCOUNT );
}

inline void loop_NES()
{
    noInterrupts();
    read_shiftRegister< SNES_LATCH , SNES_DATA , SNES_CLOCK >( NES_BITCOUNT );
    interrupts();
    sendRawData( 0 , NES_BITCOUNT );
}

/* Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, use_ssid);
  EEPROM.get(0+sizeof(use_ssid), use_password);
  char ok[2+1];
  EEPROM.get(0+sizeof(use_ssid)+sizeof(use_password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    use_ssid[0] = 0;
    use_password[0] = 0;
  }

}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, use_ssid);
  EEPROM.put(0+sizeof(use_ssid), use_password);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(use_ssid)+sizeof(use_password), ok);
  EEPROM.commit();
  EEPROM.end();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino sketch main loop definition.
void loop()
{
#ifdef MODE_GC
    loop_gc();
#elif defined MODE_N64
    loop_N64();
#elif defined MODE_SNES
    loop_SNES();
#elif defined MODE_NES
    loop_NES();
#elif defined MODE_DETECT
    if( PIN_READ( MODEPIN_GC ) ) {
        loop_gc();
    } else {
        loop_N64();
    }
#endif

    //webSocket.loop();                           // constantly check for websocket events
    ArduinoOTA.handle();                        //and updates.
    serial_commands_.ReadSerial();
    if(millis()- heartbeatMillis > 10000){
      //Serial.println("SENDING HEARTBEAT");
      if (!webSocket.sendPing(0)){
        Serial.println("Disconecting, Missed ping");
        webSocket.disconnect();
      }
      heartbeatMillis = millis();
    }

}
