#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include "secrets.h"

#define PIN_READ( pin )  GPIP(pin)
#define PINC_READ( pin ) digitalRead(pin)



#define MODEPIN_SNES 0
#define MODEPIN_N64  1
#define MODEPIN_GC   2

#define N64_PIN        5
#define N64_PREFIX     9
#define N64_BITCOUNT  33

#define SNES_LATCH      3
#define SNES_DATA       4
#define SNES_CLOCK      6
#define SNES_BITCOUNT  16
#define NES_BITCOUNT    8

#define GC_PIN        5
#define GC_PREFIX    25
#define GC_BITCOUNT  64

#define ZERO  '0'  // Use a byte value of 0x00 to represent a bit with value 0.
#define ONE    '1'  // Use an ASCII one to represent a bit with value 1.  This makes Arduino debugging easier.
#define SPLIT '\n'  // Use a new-line character to split up the controller state packets.


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NintendoSpy Firmware for Arduino
// v1.0.1
// Written by jaburns


// ---------- Uncomment one of these options to select operation mode --------------
#define MODE_GC
//#define MODE_N64
//#define MODE_SNES
//#define MODE_NES
// Bridge one of the analog GND to the right analog IN to enable your selected mode
//#define MODE_DETECT
// ---------------------------------------------------------------------------------

ESP8266WiFiMulti WiFiMulti;
WebSocketsServer webSocket = WebSocketsServer(18881);

// Declare some space to store the bits we read from a controller.
unsigned char rawData[ 128 ];
char rawData2[ 128 ];
unsigned char defined_bits;
bool trans_pending = false;
unsigned char GC_PREFIX_STRING[25] = {0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1};
unsigned char N64_PREFIX_STRING[9] = {0,0,0,0,0,0,0,1,1};

#define WAIT_FALLING_EDGE( pin,timeoutus )  while( !PIN_READ(pin) ); unsigned long sMicros = micros(); while( PIN_READ(pin) ) { if ((micros() - sMicros) > timeoutus){  memset(rawData,0,128); return;}}


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
  //if (trans_pending) return;  //need to get speed count
  //trans_pending = false;
  unsigned char *rawDataPtr = rawData;
  *rawDataPtr = PIN_READ(5);
  detachInterrupt(digitalPinToInterrupt(5));  //remove interrupt to not have a second one queue 
  ++rawDataPtr;
 
  int bits = defined_bits;
  bits--;

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
    delayMicroseconds(1);
    // Read a bit from the line and store as a byte in "rawData"
    *rawDataPtr = PIN_READ(5);
    ++rawDataPtr;
  } 
  while(--bits != 0 );
  trans_pending = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General initialization, just sets all pins to input and starts serial communication.
void setup()
{
   Serial.begin( 115200 );

   Serial.println("Connecting");
   WiFiMulti.addAP(SECRETSSID, SECRETPASSKEY);

   while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
   }
   Serial.print("Connected: ");
   Serial.println(WiFi.localIP());
   webSocket.begin();
   webSocket.onEvent(webSocketEvent);
   ArduinoOTA.begin();
   ArduinoOTA.setHostname("NintendoSpy");
   pinMode(N64_PIN,INPUT);
   pinMode(GC_PIN,INPUT);
   attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
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
    defined_bits = GC_PREFIX + GC_BITCOUNT;
    if (trans_pending){
        if (memcmp(rawData,GC_PREFIX_STRING,sizeof(GC_PREFIX_STRING)) == 0){  //Check that we recieved a packet validly
                for( unsigned char i = GC_PREFIX ; i < GC_PREFIX + GC_BITCOUNT; i++ ) {
                rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                
                }
            webSocket.broadcastTXT(rawData+GC_PREFIX,GC_BITCOUNT);
            }
            else {
              for( unsigned char i = 0 ; i < GC_PREFIX + GC_BITCOUNT; i++ ) {
                    rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                    //Serial.write(rawData[i]);
               }
               Serial.print("Bad Frame: "); Serial.printf("%s",rawData); Serial.println(' ');
            }
            memset(rawData,0,sizeof(rawData)); //Clear frame incase we got bad frame
            trans_pending = false;
            attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
    }
    unsigned long startmillis = micros();
    while((micros()-startmillis) < 100 ){
        if(!digitalRead(5)){
          startmillis = micros();
        }
    }
}

inline void loop_N64()
{
    defined_bits = N64_PREFIX + N64_BITCOUNT;
    if (trans_pending){
        if (memcmp(rawData,N64_PREFIX_STRING,sizeof(N64_PREFIX_STRING)) == 0){  //Check that we recieved a packet validly
           //Serial.print("Good Frame: ");
                for( unsigned char i = N64_PREFIX ; i < N64_PREFIX +N64_BITCOUNT; i++ ) {
                    rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                    //Serial.write(rawData[i]);
                }
           // Serial.println(SPLIT);
            webSocket.broadcastTXT(rawData+N64_PREFIX,N64_BITCOUNT);
            }
            else {
              for( unsigned char i = 0 ; i < N64_PREFIX +N64_BITCOUNT; i++ ) {
                    rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                    //Serial.write(rawData[i]);
               }
               Serial.print("Bad Frame: "); Serial.printf("%s",rawData); Serial.println(' ');
            }
            memset(rawData,0,sizeof(rawData)); //Clear frame incase we got bad frame
            trans_pending = false;
            attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
    }
    unsigned long startmillis = micros();
    while((micros()-startmillis) < 100 ){
        if(!digitalRead(5)){
          startmillis = micros();
        }
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
    if( !PINC_READ( MODEPIN_SNES ) ) {
        loop_SNES();
    } else if( !PINC_READ( MODEPIN_N64 ) ) {
        loop_N64();
    } else if( !PINC_READ( MODEPIN_GC ) ) {
        loop_GC();
    } else {
        loop_NES();
    }
#endif

    webSocket.loop();                           // constantly check for websocket events
    ArduinoOTA.handle();                        //and updates.
}
