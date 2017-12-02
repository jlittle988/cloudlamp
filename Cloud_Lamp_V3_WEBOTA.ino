#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include <FastLED.h>

#define LED_PIN     7
#define NUM_LEDS    7
#define BRIGHTNESS  255
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];


#define UPDATES_PER_SECOND 10
#define REQTOPIC "1412194/cloudlamp/statereq" //mqtt topic to publish a state request to
#define STATETOPIC "1412194/cloudlamp/state" //mqtt topic to listen for a response to state request
#define USE_MQTT true
#define WIFI_RETRY true //keep retrying wifi connection if failed
#define CYCLETHROUGH false //automatically cycle through states every stateDelay

#define STAT_LED LED_BUILTIN //LED to blink at bootup/wifi error/mqtt error
#define LEDON LOW
#define LEDOFF HIGH


CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern const TProgmemPalette16 HappyCloud_p PROGMEM;
extern const TProgmemPalette16 Sun_p PROGMEM;
extern const TProgmemPalette16 Red_p PROGMEM;
extern const TProgmemPalette16 Error_p PROGMEM;
extern const TProgmemPalette16 Lightning_p PROGMEM;

const char* ssid = "justanothernetwork"; //change these
const char* password = "wifipassword";
const char* mqtt_server = "iot.eclipse.org";
const char* host = "cloudlamp-webupdate";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

char state = '3'; 

int lightningTimer = 0;
int stateTimer = 0;
int stateDelay = 360000;
int pollTimer = 0;
int pollDelay = 10000;

uint8_t pulseBri = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  state = (char)payload[0];

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "CloudLamp";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    
      client.subscribe(STATETOPIC);
    } else
    {
      Serial.print("failed, rc=");
      MQTT_Error();
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

    //this stuff is for OTA Updates
    
    MDNS.begin(host);

    httpUpdater.setup(&httpServer);
    httpServer.begin();

    MDNS.addService("http", "tcp", 80);

  //this is for connecting to wifi
      
  pinMode(STAT_LED, OUTPUT);
  digitalWrite(STAT_LED, LEDOFF);
  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    Wifi_Error();
    if(WIFI_RETRY)
    {
      delay(5000);
      ESP.restart();
    }
  }

  

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //delay( 3000 ); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );

    
}


void loop()
{

   httpServer.handleClient(); //for ota

    
    if (!client.connected() && USE_MQTT) {
    reconnect();
    }
    if(USE_MQTT) client.loop();

    /*
    if(WiFi.status() != WL_CONNECTED) 
    {
      Wifi_Error();
      
    }
    */
    
    
    
    currentBlending = LINEARBLEND;

    //Serial.println(sizeof(currentPalette));
    
    static uint8_t startIndex = 0;
    if(state == '1') HappyCloud(startIndex, startIndex); else if(state == '2') ThunderCloud(); else if(state == '3') SunnyCloud(startIndex, startIndex); else if(state == '9') RedCloud(startIndex); else Off();
    if(CYCLETHROUGH)
    {
    if(stateTimer<=millis())
    {
      if(state=='1') state='2'; else if(state=='2') state='1';
      stateTimer=millis()+stateDelay;
    }
    }
    else
    {
      if(pollTimer<=millis())
      {
        client.publish(REQTOPIC, "polling");
        pollTimer=millis()+pollDelay;
      }
    }
    
    
    startIndex = startIndex + 1; /* motion speed */
    
    
    
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
    
}

void ThunderCloud()
{
  for(int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  if (millis() >= lightningTimer)
  {
    int led = random(0, NUM_LEDS);
    BlinkPixel(led, 10, 10);

    if(random(0,6)<=3) lightningTimer = millis()+random(0, 300); else lightningTimer = millis()+random(3000, 10000);
  }
}

void BlinkPixel (int pix, int inc, int del)
{
  for (int i = 0; i<255; i+=inc)
  {
    leds[pix] = ColorFromPalette( Lightning_p, random(0,32), i, LINEARBLEND);
    FastLED.delay(10);
  }
  FastLED.delay(del);
  for (int i = 0; i>0; i-=inc)
  {
    leds[pix] = ColorFromPalette( Lightning_p, random(0,32), i, LINEARBLEND);
    FastLED.delay(10);
  }
  leds[pix] = CRGB::Black;
}

void HappyCloud( uint8_t colorIndex, uint8_t colorIndex2)
{
    uint8_t brightness = 255;

    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( HappyCloud_p, colorIndex, brightness, LINEARBLEND);
        colorIndex += 8;
        
    }

    //leds[3] = ColorFromPalette( Sun_p, colorIndex2, constrain(abs(127-pulseBri)*2, 40, 255), LINEARBLEND);
    colorIndex2 += 8;
    pulseBri += 1;
}

void SunnyCloud( uint8_t colorIndex, uint8_t colorIndex2)
{
    uint8_t brightness = 255;

    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( HappyCloud_p, colorIndex, brightness, LINEARBLEND);
        //leds[i] = CRGB::Black;
        colorIndex += 8;
        
    }

    leds[2] = ColorFromPalette( Sun_p, colorIndex2, 255, LINEARBLEND);
    colorIndex2 += 8;
}

void Off()
{
    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        
    }
}

void Wifi_Error()
{
    /*
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( Error_p, 0, constrain(abs(127-pulseBri)*2, 0, 255), NOBLEND);
        pulseBri+=1;
        
    }
    */

    digitalWrite(STAT_LED, LEDON);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDOFF);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDON);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDOFF);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDON);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDOFF);
    FastLED.delay(1000);
    
    
}

void MQTT_Error()
{
    /*
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( Error_p, 16, constrain(abs(127-pulseBri)*2, 0, 255), NOBLEND);
        pulseBri+=1;
        
    }
    */

    digitalWrite(STAT_LED, LEDON);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDOFF);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDON);
    FastLED.delay(200);
    digitalWrite(STAT_LED, LEDOFF);
    FastLED.delay(1000);
}

void RedCloud( uint8_t colorIndex)
{
    uint8_t brightness = 255;

    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( Red_p, colorIndex, brightness, LINEARBLEND);
        colorIndex += 8;
        
    }
 
}



const TProgmemPalette16 Lightning_p PROGMEM =
{
  CRGB::White,
  CRGB::Thistle
};

const TProgmemPalette16 HappyCloud_p PROGMEM =
{
    CRGB::LightSteelBlue,
    CRGB::SkyBlue,
    CRGB::White,
    CRGB::SteelBlue,

    CRGB::SkyBlue,
    CRGB::Grey,
    CRGB::White,
    CRGB::Grey,  

    CRGB::PowderBlue,
    CRGB::SteelBlue,
    CRGB::White,
    CRGB::LightSteelBlue,  

    CRGB::SkyBlue,
    CRGB::SteelBlue,
    CRGB::SteelBlue,  
    CRGB::White
    
};

const TProgmemPalette16 Sun_p PROGMEM =
{
  CRGB::Yellow,
  CRGB::Gold,

  CRGB::Yellow,
  CRGB::Gold,

  CRGB::Yellow,
  CRGB::Gold,

  CRGB::Yellow,
  CRGB::Gold,

  
  CRGB::Yellow,
  CRGB::Gold,

  CRGB::Yellow,
  CRGB::Gold,

  CRGB::Yellow,
  CRGB::Gold,

  CRGB::Yellow,
  CRGB::Gold
  
};

const TProgmemPalette16 Red_p PROGMEM =
{
  CRGB::Black,
  CRGB::Red,
  CRGB::FireBrick,
  CRGB::DarkRed,
  
  CRGB::Red,
  CRGB::Red,
  CRGB::FireBrick,
  CRGB::Black,
  
  CRGB::Red,
  CRGB::Black,
  CRGB::FireBrick,
  CRGB::DarkRed,
  
  CRGB::Red,
  CRGB::Red,
  CRGB::Black,
  CRGB::DarkRed
};

const TProgmemPalette16 Error_p PROGMEM =
{
  CRGB::Red, //WiFi Error
  CRGB::Gold //MQTT Error
};

// Additionl notes on FastLED compact palettes:
//
// Normally, in computer graphics, the palette (or "color lookup table")
// has 256 entries, each containing a specific 24-bit RGB color.  You can then
// index into the color palette using a simple 8-bit (one byte) value.
// A 256-entry color palette takes up 768 bytes of RAM, which on Arduino
// is quite possibly "too many" bytes.
//
// FastLED does offer traditional 256-element palettes, for setups that
// can afford the 768-byte cost in RAM.
//
// However, FastLED also offers a compact alternative.  FastLED offers
// palettes that store 16 distinct entries, but can be accessed AS IF
// they actually have 256 entries; this is accomplished by interpolating
// between the 16 explicit entries to create fifteen intermediate palette
// entries between each pair.
//
// So for example, if you set the first two explicit entries of a compact 
// palette to Green (0,255,0) and Blue (0,0,255), and then retrieved 
// the first sixteen entries from the virtual palette (of 256), you'd get
// Green, followed by a smooth gradient from green-to-blue, and then Blue.
