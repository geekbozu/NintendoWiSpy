#include <Arduino.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <SerialCommands.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ets_sys.h>
#include <WebSocketsServer.h>
#include <ESP8266NetBIOS.h>
#include <ESP8266LLMNR.h>
#include <ESP8266SSDP.h>

//#include <rom/ets_sys.h>

ADC_MODE(ADC_VCC);
#define PIN_READ(pin) GPIP(pin)
#define PINC_READ(pin) digitalRead(pin)

#define MODEPIN_SNES 0
#define MODEPIN_N64 1
#define MODEPIN_GC 13
#define BIT_PIN 14
#define FRAME_PIN 15

#define N64_PIN 5
#define N64_PREFIX 9
#define N64_BITCOUNT 33

#define SNES_LATCH 3
#define SNES_DATA 4
#define SNES_CLOCK 6
#define SNES_BITCOUNT 16
#define NES_BITCOUNT 8

#define GC_PIN 5
#define GC_PREFIX 25
#define GC_BITCOUNT 65

#define ZERO '0'   // Use a byte value of 0x00 to represent a bit with value 0.
#define ONE '1'    // Use an ASCII one to represent a bit with value 1.  This makes Arduino debugging easier.
#define SPLIT '\n' // Use a new-line character to split up the controller state packets.

#define HEARTBEATMSG "pong"
#define WAIT_FALLING_EDGE(pin, timeoutus)     \
    while (!PIN_READ(pin))                    \
        ;                                     \
    unsigned long sMicros = micros();         \
    while (PIN_READ(pin))                     \
    {                                         \
        if ((micros() - sMicros) > timeoutus) \
        {                                     \
            memset(rawData, 0, 128);          \
            return;                           \
        }                                     \
    }

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

//Controller packet headers MINUS the first bit
const unsigned char GC_PREFIX_STRING[25] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
                                       //     B  1  0  0  0  0  0  0  0  0  0  0  0  0  1  1  0  0  0  0  0  0  0  0  1
//const unsigned char GC_PREFIX_STRING[24] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
const unsigned char N64_PREFIX_STRING[9] = {0, 0, 0, 0, 0, 0, 0, 1, 1};

// Declare some space to store the bits we read from a controller.
volatile unsigned char defined_bits = 1;
unsigned char rawData[128];

// Runtime state variables
bool trans_pending = false;
bool use_serial = false;

unsigned long heartbeatMillis;
unsigned long mdnsMillis;


bool conActive = false;
int tog = 0; //this is so bad

#define CPU_FREQ 160000000L
#define CPU_US_CYCLE (CPU_FREQ/1000000)
#define CPU_MS_CYCLE (CPU_FREQ/1000)



char serial_command_buffer_[256];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\n", " ");
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WebSocketsServer webSocket = WebSocketsServer(18881, "", "");

//This is the default handler, and gets called when no other command matches.
void cmd_unrecognized(SerialCommands *sender, const char *cmd)
{
    sender->GetSerial()->print("Unrecognized command [");
    sender->GetSerial()->print(cmd);
    sender->GetSerial()->println("]");
}

void cmd_rwifi(SerialCommands *sender)
{
    loadCredentials();
    sender->GetSerial()->println(use_ssid);
    sender->GetSerial()->println(use_password);
}

void cmd_wifi(SerialCommands *sender)
{
    char *ssid = sender->Next();
    char *password = sender->Next();
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
    strcpy(use_ssid, ssid);
    saveCredentials();
}

void cmd_tserial(SerialCommands *sender)
{
    char *s = sender->Next();
    if (s == NULL)
    {
        sender->GetSerial()->println("BAD MODE");
        return;
    }
    if (s[0] == '0')
    {
        use_serial = false;
        sender->GetSerial()->println("WEB SOCKET MODE");
        return;
    }
    if (s[0] == '1')
    {
        use_serial = true;
        sender->GetSerial()->println("SERIAL MODE");
    }
}
SerialCommand cmd_wifi_write_("WW", cmd_wifi);
SerialCommand cmd_wifi_read_("RW", cmd_rwifi);
SerialCommand cmd_tog_ser_("TS", cmd_tserial);

/* Load WLAN credentials from EEPROM */
void loadCredentials()
{
    EEPROM.begin(512);
    EEPROM.get(0, use_ssid);
    EEPROM.get(0 + sizeof(use_ssid), use_password);
    char ok[2 + 1];
    EEPROM.get(0 + sizeof(use_ssid) + sizeof(use_password), ok);
    EEPROM.end();
    if (String(ok) != String("OK"))
    {
        use_ssid[0] = 0;
        use_password[0] = 0;
    }
}

/** Store WLAN credentials to EEPROM */
void saveCredentials()
{
    EEPROM.begin(512);
    EEPROM.put(0, use_ssid);
    EEPROM.put(0 + sizeof(use_ssid), use_password);
    char ok[2 + 1] = "OK";
    EEPROM.put(0 + sizeof(use_ssid) + sizeof(use_password), ok);
    EEPROM.commit();
    EEPROM.end();
}

/** WebSocketServer event handler */
void IRAM_ATTR webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

    switch (type)
    {
    case WStype_DISCONNECTED: //
    {
     //   IPAddress ip = webSocket.remoteIP(num);
      //  Serial.printf("[%u] Disconnected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        
        break;
    }
    case WStype_CONNECTED:
    {
     //  IPAddress ip = webSocket.remoteIP(num);
      //  Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
    }
    break;

    case WStype_TEXT:
        //Serial.printf("[%u] get Text: %s\n", num, payload);
        if (memcmp(payload, "ping", 4) == 0)
        {
            webSocket.sendTXT(num, HEARTBEATMSG);
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

void IRAM_ATTR handleRoot()
{ // When URI / is requested, send a web page with a button to toggle the LED
    httpServer.send(200, "text/html",
                    "<form action=\"/do?action=ZERO\" method=\"POST\"><input type=\"submit\" value=\"Zero Joystick\"></form>"
                    "<form action=\"/do?action=REFRESH\" method=\"POST\"><input type=\"submit\" value=\"Refresh UI\"></form>"
                    "<form action=\"/do?action=CLEAR\" method=\"POST\"><input type=\"submit\" value=\"Clear\"></form>"
                    "<form action=\"/do?action=COUNTS\" method=\"POST\"><input type=\"submit\" value=\"Counts\"></form>"
                    "<form action=\"/update\" method=\"get\"><input type=\"submit\" value=\"Update\"></form>");
}
void IRAM_ATTR handleAction()
{ // If a POST request is made to URI /LED
    String tmp = httpServer.arg("action");
    if(tmp.equals("ZERO")){
        pinMode(5,OUTPUT);
        digitalWrite(5,LOW);
        delay(10);
        pinMode(5,INPUT);
    }
    webSocket.broadcastTXT(tmp);
    httpServer.sendHeader("Location", "/"); // Add a header to respond with a new location for the browser to go to the home page again
    httpServer.send(303);                   // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

/** UNUSED CODE FOR NOW */
/*
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Preferred method for reading SNES + NES controller data.
template< unsigned char latch, unsigned char data, unsigned char clock >
void read_shiftRegister( unsigned char bits ) {
    unsigned char *rawDataPtr = rawData;

    WAIT_FALLING_EDGE( latch, 10 );

    do {
        WAIT_FALLING_EDGE( clock, 10 );
        *rawDataPtr = !PIN_READ(data);
        ++rawDataPtr;
    } while ( --bits > 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sends a packet of controller data over the Arduino serial interface.
inline void sendRawData( unsigned char first, unsigned char count ) {
    for ( unsigned char i = first ; i < first + count ; i++ ) {
        Serial.write( rawData[i] ? ONE : ZERO );
    }
    Serial.write( SPLIT );
}

inline void loop_SNES() {
    noInterrupts();
    read_shiftRegister< SNES_LATCH , SNES_DATA , SNES_CLOCK >( SNES_BITCOUNT );
    interrupts();
    sendRawData( 0 , SNES_BITCOUNT );
}

inline void loop_NES() {
    noInterrupts();
    read_shiftRegister< SNES_LATCH , SNES_DATA , SNES_CLOCK >( NES_BITCOUNT );
    interrupts();
    sendRawData( 0 , NES_BITCOUNT );
}

*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Update loop definitions for the various console modes.

void IRAM_ATTR loop_gc()
{

    uint32_t start = ESP.getCycleCount();
    uint32_t b = ESP.getCycleCount();

    digitalWrite(FRAME_PIN, 1);
    while (1)
    {
        if(PIN_READ(5) == 0)
        {
            start = ESP.getCycleCount();
        }

        if ((ESP.getCycleCount() - start) > (CPU_US_CYCLE*100))
        {
            break;
        }
        if ((ESP.getCycleCount() - b) > (CPU_MS_CYCLE))
        {
            digitalWrite(FRAME_PIN, 0);
            Serial.print("NAYNAY\r\n");
            return;
        }
        //Serial.print(ESP.getCycleCount()-b);

    }
    ets_intr_lock();
    
    memset(rawData, 0, sizeof(rawData)); //Clear frame incase we got bad frame
    trans_pending = false;
    defined_bits = GC_PREFIX + GC_BITCOUNT;
    uint32_t clock;
    unsigned char *rawDataPtr = rawData;
    while (defined_bits != 0)
    {
        digitalWrite(BIT_PIN, 1);

        // Wait ~2us between line reads

        while (PIN_READ(5)) //Wait for low transition
        {
        }
        clock = ESP.getCycleCount();
        while (!PIN_READ(5)) //Wait for High transition
        {
        }

        if ((ESP.getCycleCount() - clock) > (CPU_US_CYCLE*2))
        { //If Low for more then 1us
            *rawDataPtr = 0;
        }
        else
        {
            *rawDataPtr = 1;
        }

        digitalWrite(BIT_PIN, 0);
        ++rawDataPtr;
        defined_bits--;
    }
    trans_pending = true;
    digitalWrite(FRAME_PIN, 0);
    ets_intr_unlock();
    
    if (trans_pending)
    {
        rawData[23] = 0;
        if (memcmp(rawData, GC_PREFIX_STRING, sizeof(GC_PREFIX_STRING)) == 0)
        { //Check that we recieved a packet validly
            if (use_serial)
            {
                for (unsigned char i = GC_PREFIX; i < GC_PREFIX + GC_BITCOUNT; i++)
                {
                    rawData[i] = (rawData[i] ? ONE : ZERO); //Compat with Original hardware
                    if (use_serial)
                    {
                        Serial.write(rawData[i]);
                    }
                }
                Serial.write(SPLIT);
            }
            else
            {
               webSocket.broadcastTXT(rawData + GC_PREFIX, GC_BITCOUNT - 1);

            }

        }
        else 
        {
            for ( unsigned char i = 0 ; i < GC_PREFIX + GC_BITCOUNT; i++ ) {
                rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                //Serial.write(rawData[i]);
            
            }
            
            if(rawData[0]==ONE){
                rawData[0] = 'B';
            }else{
                rawData[0] = 'b';
            }
            if(rawData[1]==ONE){
                rawData[1] = 'G';
            }else{
                rawData[1] = 'g';
            }            
            webSocket.broadcastTXT(rawData, GC_BITCOUNT+GC_PREFIX );
            
            //Serial.print("Bad GCN Frame: "); Serial.printf("%s"rawData); Serial.println(' ');
        }
        memset(rawData, 0, sizeof(rawData)); //Clear frame incase we got bad frame
        trans_pending = false;
    }
}

void IRAM_ATTR loop_N64()
{
 
    uint32_t start = ESP.getCycleCount();
    uint32_t b = ESP.getCycleCount();

    digitalWrite(FRAME_PIN, 1);
    while (1)
    {
        if(PIN_READ(5) == 0)
        {
            start = ESP.getCycleCount();
        }

        if ((ESP.getCycleCount() - start) > (CPU_US_CYCLE*100))
        {
            break;
        }
        if ((ESP.getCycleCount() - b) > (CPU_MS_CYCLE))
        {
            digitalWrite(FRAME_PIN, 0);
            Serial.print("NAYNAY\r\n");
            return;
        }
        //Serial.print(ESP.getCycleCount()-b);

    }
    ets_intr_lock();
    
    memset(rawData, 0, sizeof(rawData)); //Clear frame incase we got bad frame
    trans_pending = false;
    defined_bits = N64_PREFIX + N64_BITCOUNT;
    uint32_t clock;
    unsigned char *rawDataPtr = rawData;
    while (defined_bits != 0)
    {
        

        // Wait ~2us between line reads

        while (PIN_READ(5)) //Wait for low transition
        {
        }
        digitalWrite(BIT_PIN, 1);
        clock = ESP.getCycleCount();
        while (!PIN_READ(5)) //Wait for High transition
        {
        }
        digitalWrite(BIT_PIN, 0);
        if ((ESP.getCycleCount() - clock) > (CPU_US_CYCLE*1.5))
        { //If Low for more then 1us
            *rawDataPtr = 0;
        }
        else
        {
            *rawDataPtr = 1;
        }

       // digitalWrite(BIT_PIN, 0);
        ++rawDataPtr;
        defined_bits--;
    }
    trans_pending = true;
    digitalWrite(FRAME_PIN, 0);
    ets_intr_unlock();
    
    if (trans_pending)
    {
        //rawData[23] = 0;
        if (memcmp(rawData, N64_PREFIX_STRING, sizeof(N64_PREFIX_STRING)) == 0)
        { //Check that we recieved a packet validly
            if (use_serial)
            {
                for (unsigned char i = N64_PREFIX; i < N64_PREFIX + N64_BITCOUNT; i++)
                {
                    rawData[i] = (rawData[i] ? ONE : ZERO); //Compat with Original hardware
                    if (use_serial)
                    {
                        Serial.write(rawData[i]);
                    }
                }
                Serial.write(SPLIT);
            }
            else
            {
               webSocket.broadcastTXT(rawData + N64_PREFIX, N64_BITCOUNT - 1);

            }

        }
        else 
        {
            for ( unsigned char i = 0 ; i < N64_PREFIX + N64_BITCOUNT; i++ ) {
                rawData[i] = (rawData[i] ? ONE : ZERO );  //Compat with Original hardware
                //Serial.write(rawData[i]);
            
            }
            
            if(rawData[0]==ONE){
                rawData[0] = 'B';
            }else{
                rawData[0] = 'b';
            }
            if(rawData[1]==ONE){
                rawData[1] = 'N';
            }else{
                rawData[1] = 'n';
            }
            
            webSocket.broadcastTXT(rawData, N64_BITCOUNT+N64_PREFIX );
            
            //Serial.print("Bad GCN Frame: "); Serial.printf("%s"rawData); Serial.println(' ');
        }
        memset(rawData, 0, sizeof(rawData)); //Clear frame incase we got bad frame
        trans_pending = false;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General initialization, just sets all pins to input and starts serial communication.
void setup()
{

    pinMode(N64_PIN, INPUT);
    pinMode(MODEPIN_GC, INPUT);
    pinMode(BIT_PIN, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(FRAME_PIN, OUTPUT);
    pinMode(2,OUTPUT);
    digitalWrite(2,HIGH);
    Serial.begin(921600);
    Serial.print(ESP.getResetInfo());
    Serial.print("\r\n");
    serial_commands_.SetDefaultHandler(cmd_unrecognized);
    serial_commands_.AddCommand(&cmd_wifi_read_);
    serial_commands_.AddCommand(&cmd_wifi_write_);
    serial_commands_.AddCommand(&cmd_tog_ser_);
    Serial.println("Connecting");
    WiFi.persistent(false);
    WiFi.hostname("NintendoWiSpy");
    loadCredentials();
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    //WiFi.begin(use_ssid, use_password);
    Serial.printf("%s : %s",use_ssid,use_password);
    WiFi.begin(use_ssid, use_password);
    while (WiFi.status() != WL_CONNECTED)
    {
        serial_commands_.ReadSerial();
        Serial.printf("WiFi Status:[ %u]\n", WiFi.status());
        delay(1000);
        ESP.wdtFeed();
    }
    Serial.print("Connected: ");
    digitalWrite(2,LOW);
    Serial.println(WiFi.localIP());
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    httpUpdater.setup(&httpServer);
    httpServer.begin();
    httpServer.on("/", HTTP_GET, handleRoot);
    httpServer.on("/index.html", HTTP_GET, handleRoot);
    httpServer.on("/do", HTTP_POST, handleAction);
    
    NBNS.begin("NintendoWiSpy");
    LLMNR.begin("NintendoWiSpy");
    httpServer.on("/description.xml", HTTP_GET, []() {
      SSDP.schema(httpServer.client());
    });
    SSDP.setHTTPPort(80);
    SSDP.setName("NintendoWiSpy");
    SSDP.setURL("/");
    SSDP.setSchemaURL("description.xml");
    SSDP.setModelName("NintendoWiSpy Input Display");
    SSDP.setModelNumber("U Wot M8 V1");
    SSDP.setModelURL("https://keepdream.in");
    SSDP.setManufacturer("Timothy 'Geekboy1011' Keller");
    SSDP.setManufacturerURL("https://keepdream.in");
    SSDP.setSerialNumber(ESP.getChipId());
    SSDP.setTTL(1);
    SSDP.setDeviceType("upnp:rootdevice");
    SSDP.begin();

   // MDNS.begin("NintendoWiSpy");
   // MDNS.addService("http", "tcp", 18881); // Add websocket port.
   // MDNS.addService("http", "tcp", 80);    // Add http update server
   // MDNS.update();
    heartbeatMillis = millis();
    mdnsMillis = millis();
    ESP.wdtEnable(WDTO_8S);
    
    Serial.printf("VCC:[ %u]\r\n", ESP.getVcc());

    
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino sketch main loop definition.
void IRAM_ATTR loop()
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
    if (PIN_READ(MODEPIN_GC))
    {
        loop_gc();
    }
    else
    {
        loop_N64();
    }
#endif
    
    webSocket.loop();
    delay(1);
   //
    if ((millis() - heartbeatMillis) > 60000)
    { // Every 10 seconds
        char RSSI[10];
        sprintf(RSSI, "RSSI: %d", WiFi.RSSI());
        webSocket.broadcastTXT(RSSI);
        heartbeatMillis = millis();
            Serial.printf("VCC:[ %u]\r\n", ESP.getVcc());
    Serial.printf("MEM: %d,%d\r\n",ESP.getFreeHeap(),ESP.getHeapFragmentation());
    Serial.printf("WiFi Status:[ %u]\r\n", WiFi.status());
    digitalWrite(2,HIGH);
    }
  /*  if ((millis() - mdnsMillis) > 36000000)
    {
        MDNS.end();
        MDNS.begin("NintendoWiSpy");
        MDNS.addService("http", "tcp", 18881); // Add websocket port.
        MDNS.addService("http", "tcp", 80);    // Add http update server
        MDNS.update();
        mdnsMillis = millis();
    }
    MDNS.update();*/
    yield();
    httpServer.handleClient();
    
    yield();
    ESP.wdtFeed();
    yield();
    serial_commands_.ReadSerial();
    yield();
    /*
    Serial.print(ESP.getResetInfo());
    Serial.print("\r\n");
    int c=webSocket.connectedClients();
    Serial.printf("Connected Client: %d\r\n",c);
    for(int i=0;i<c;i++)
    {
        IPAddress ip = webSocket.remoteIP(i);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\r\n", i, ip[0], ip[1], ip[2], ip[3]);
    }
    Serial.printf("VCC:[ %u]\r\n", ESP.getVcc());
    Serial.printf("MEM: %d,%d\r\n",ESP.getFreeHeap(),ESP.getHeapFragmentation());
    Serial.printf("WiFi Status:[ %u]\r\n", WiFi.status());
    */
    
}
