#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>

#define USE_SERIAL Serial
#define PIN_READ( pin )  GPIP(pin)
#define PINC_READ( pin ) digitalRead(pin)
#define MICROSECOND_NOPS "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"

#define WAIT_FALLING_EDGE( pin ) while( !PIN_READ(pin) ); while( PIN_READ(pin) );

#define MODEPIN_SNES 0
#define MODEPIN_N64  1
#define MODEPIN_GC   2

#define N64_PIN        2
#define N64_PREFIX     9
#define N64_BITCOUNT  32

#define SNES_LATCH      3
#define SNES_DATA       4
#define SNES_CLOCK      6
#define SNES_BITCOUNT  16
#define  NES_BITCOUNT   8

#define GC_PIN        5
#define GC_PREFIX    25
#define GC_BITCOUNT  64

#define ZERO  '\0'  // Use a byte value of 0x00 to represent a bit with value 0.
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
// The only reason you'd want to use 2-wire SNES mode is if you built a NintendoSpy
// before the 3-wire firmware was implemented.  This mode is for backwards
// compatibility only.
//#define MODE_2WIRE_SNES
// ---------------------------------------------------------------------------------

ESP8266WiFiMulti WiFiMulti;

WebSocketsServer webSocket = WebSocketsServer(18881);

const char *OTAName = "ESP8266_NINTENDOSPY";           // A name and a password for the OTA service
const char *OTAPassword = "secret";
const char* mdnsName = "espSpy"; // Domain name for the mDNS responder
// Declare some space to store the bits we read from a controller.
unsigned char rawData[ 128 ];
char rawData2[ 128 ];
unsigned char defined_bits;
bool trans_pending = false;
unsigned char GC_PREFIX_STRING[25] = {0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1};

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED://
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        
        // send message to client
        webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General initialization, just sets all pins to input and starts serial communication.
void setup()
{
   Serial.begin( 115200 );

   Serial.print("Connecting");
   WiFiMulti.addAP("8E4C80", "14415555");

   while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
   }
   Serial.print("Connected");
   Serial.print(WiFi.localIP());
   webSocket.begin();
   webSocket.onEvent(webSocketEvent);
   pinMode(5,INPUT);

   attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);

}
void gc_n64_isr()
{   //Takes ~2us to enter
  //if (trans_pending) return;  //need to get speed count
  unsigned char *rawDataPtr = rawData;
  *rawDataPtr = (PIN_READ(5)) ? 1 : 0;
  detachInterrupt(digitalPinToInterrupt(5));  //remove interrupt to not have a second one queue 
  ++rawDataPtr;
 
  int bits = defined_bits;
  bits--;
  
read_loop:
  do {
    // Wait for the line to go high then low.
    WAIT_FALLING_EDGE( 5 );

    // Wait ~2us between line reads
    delayMicroseconds(2);
    // Read a bit from the line and store as a byte in "rawData"
    *rawDataPtr = (PIN_READ(5)) ? 1 : 0;
    ++rawDataPtr;
  } 
  while(--bits != 0 );
  trans_pending = true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Performs a read cycle from one of Nintendo's one-wire interface based controllers.
// This includes the N64 and the Gamecube.
//     pin  = Pin index on Port D where the data wire is attached.
//     bits = Number of bits to read from the line.
template< unsigned char pin >
void read_oneWire( unsigned char bits )
{
    
bits = 0;

}

// Verifies that the 9 bits prefixing N64 controller data in 'rawData'
// are actually indicative of a controller state signal.
inline bool checkPrefixN64 ()
{
    if( rawData[0] != 0 ) return false; // 0
    if( rawData[1] != 0 ) return false; // 0
    if( rawData[2] != 0 ) return false; // 0
    if( rawData[3] != 0 ) return false; // 0
    if( rawData[4] != 0 ) return false; // 0
    if( rawData[5] != 0 ) return false; // 0
    if( rawData[6] != 0 ) return false; // 0
    if( rawData[7] == 0 ) return false; // 1
    if( rawData[8] == 0 ) return false; // 1
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Performs a read cycle from a shift register based controller (SNES + NES) using only the data and latch
// wires, and waiting a fixed time between reads.  This read method is deprecated due to being finicky,
// but still exists here to support older builds.
//     latch = Pin index on Port D where the latch wire is attached.
//     data  = Pin index on Port D where the output data wire is attached.
//     bits  = Number of bits to read from the controller.
//  longWait = The NES takes a bit longer between reads to get valid results back.
template< unsigned char latch, unsigned char data, unsigned char longWait >
void read_shiftRegister_2wire( unsigned char bits )
{
    unsigned char *rawDataPtr = rawData;

    WAIT_FALLING_EDGE( latch );

read_loop:

    // Read the data from the line and store in "rawData"
    *rawDataPtr = !PIN_READ(data);
    ++rawDataPtr;
    if( --bits == 0 ) return;

    // Wait until the next button value is on the data line. ~12us between each.
    asm volatile(
        MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS
        MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS
        MICROSECOND_NOPS MICROSECOND_NOPS
    );
    if( longWait ) {
        asm volatile(
            MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS
            MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS MICROSECOND_NOPS
            MICROSECOND_NOPS MICROSECOND_NOPS
        );
    }

    goto read_loop;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Preferred method for reading SNES + NES controller data.
template< unsigned char latch, unsigned char data, unsigned char clock >
void read_shiftRegister( unsigned char bits )
{
    unsigned char *rawDataPtr = rawData;

    WAIT_FALLING_EDGE( latch );

    do {
        WAIT_FALLING_EDGE( clock );
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

inline void loop_GC()
{
    noInterrupts();
    read_oneWire< GC_PIN >( GC_PREFIX + GC_BITCOUNT );
    interrupts();
    if( rawData[25] != 0 ) return;
    if( rawData[26] != 0 ) return;
    sendRawData( GC_PREFIX , GC_BITCOUNT );
}

inline void loop_N64()
{
    noInterrupts();
    read_oneWire< N64_PIN >( N64_PREFIX + N64_BITCOUNT );
    interrupts();
    if( checkPrefixN64() ) {
        sendRawData( N64_PREFIX , N64_BITCOUNT );
    }
}

inline void loop_SNES()
{
    noInterrupts();
#ifdef MODE_2WIRE_SNES
    read_shiftRegister_2wire< SNES_LATCH , SNES_DATA , false >( SNES_BITCOUNT );
#else
    read_shiftRegister< SNES_LATCH , SNES_DATA , SNES_CLOCK >( SNES_BITCOUNT );
#endif
    interrupts();
    sendRawData( 0 , SNES_BITCOUNT );
}

inline void loop_NES()
{
    noInterrupts();
#ifdef MODE_2WIRE_SNES
    read_shiftRegister< SNES_LATCH , SNES_DATA , true >( NES_BITCOUNT );
#else
    read_shiftRegister< SNES_LATCH , SNES_DATA , SNES_CLOCK >( NES_BITCOUNT );
#endif
    interrupts();
    sendRawData( 0 , NES_BITCOUNT );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino sketch main loop definition.
void loop()
{
#ifdef MODE_GC
    defined_bits = GC_PREFIX + GC_BITCOUNT;
    if (trans_pending){
        if (memcmp(rawData,GC_PREFIX_STRING,sizeof(GC_PREFIX_STRING)) == 0){  //Check that we recieved a packet validly
                for( unsigned char i = GC_PREFIX ; i < GC_PREFIX + 64; i++ ) {
                rawData2[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                }
            webSocket.broadcastTXT(rawData2+GC_PREFIX,64);
            }
    }

    trans_pending = false;
    attachInterrupt(digitalPinToInterrupt(5),gc_n64_isr,FALLING);
  
    webSocket.loop();                           // constantly check for websocket events
    //Serial.println("Loopy");

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
}
