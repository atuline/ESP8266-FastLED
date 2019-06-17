/* Fire2012 with Palette and controlled by MQTT
 *
 * By: Mark Kriegsman with MQTT controls by Andrew Tuline
 *
 * Modified Date: June, 2019
 *
 * Here's Mark Kriegsman's Fire2012withpalette along with control by MQTT Messaging. This routine is different from the standard ethernet and MQTT 
 * programming in that it runs asynchronously and won't stop the display routine if no connectivity is made. It also has a more robut reconnect capability.
 * 
 * The MQTT modification was written for ESP8266 based WeMOS D1 Mini.
 * 
 * MQTT Controls include:
 * 
 * client.subscribe("device");                            // Variable is mydevice
 * client.subscribe("bri");                               // Variable is mybri
 * client.subscribe("hue");                               // Variable is myhue
 * client.subscribe("speed");                             // Variable is myspeed
 * client.subscribe("cool");                              // Variable is mycool
 * client.subscribe("spark");                             // Variable is myspark
 *
 * The Android phone application used is called 'IoT MQTT Panel Pro'.
 * 
 */


#include <ESP8266WiFi.h>                                    // Should be included when you install the ESP8266.
#include <PubSubClient.h>                                   // https://github.com/knolleary/pubsubclient
#include "FastLED.h"                                        // FastLED library.

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif


// WiFi Authentication -------------------------------------------------------------------------
//const char* ssid = "WiFiPlus5041-2.4G";                     // WiFi configuration for home network. Default Gateway IP is 192.168.1.254
//const char* password =  "t84uw5hc29";

const char* ssid = "SM-G965W4004";                            // WiFi configuration for Android tethering network. The Android IP address is hard coded at 192.168.43.1.
const char* password = "fnig2036";


// MQTT Authentication -------------------------------------------------------------------------
//const char* mqttServer = "postman.cloudmqtt.com";           // CloudMQTT.com broker credentials.
//const int mqttPort = 16195;
//const char* mqttUser = "wmabzsyn";
//const char* mqttPassword = "GT8Do3vkgWP5";

//const char* mqttServer = "192.168.1.64";                    // Workstation Mosquitto broker credentials - Don't forget to install dependencies and allow TCP 1883 through the firewall as well as setup the user in mosquitto.conf.
//const int mqttPort = 1883;
//const char* mqttUser = "wmabzsyn";
//const char* mqttPassword = "GT8Do3vkgWP5";

//const char* mqttServer = "192.168.1.67";                  // MQTT broker application on Android credentials - using the home network WiFi.
//const int mqttPort = 1883;
//const char* mqttUser = "wmabzsyn";
//const char* mqttPassword = "GT8Do3vkgWP5";

// My Phone as a tethered device (working)                  // MQTT Broker application on Android credentials - using the Android tethered WiFi.
const char* mqttServer = "192.168.43.1";
const int mqttPort = 1883;
const char* mqttUser = "wmabzsyn";
const char* mqttPassword = "GT8Do3vkgWP5";

const char *prefixtopic = "fire/";
const char* subscribetopic[] = {"device", "bri", "hue", "speed", "cool", "spark"};

char message_buff[100];
 
WiFiClient espClient;
PubSubClient client(espClient);

//IPAddress staticIP(192, 168, 43, 10);                       // ESP static ip.
//IPAddress gateway(192, 168, 43, 1);                         // IP Address of tethered phones.
//IPAddress subnet(255, 255, 255, 0);                         // Subnet mask.
//IPAddress dns(8, 8, 8, 8);                                  // DNS.

bool wifistart = false;                                       // State variables for asynchronous wifi & mqtt connection.
bool mqttconn = false;
bool wifimqtt = false;


const uint8_t kMatrixWidth = 2;                               // Number of lengths.
const uint8_t kMatrixHeight = 11;                             // How long each length is.
const bool    kMatrixSerpentineLayout = true;                 // Using a serpentine layout.


 #define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// Fixed definitions cannot change on the fly.
#define LED_DT D5                                             // Data pin to connect to the strip.
#define COLOR_ORDER GRB                                       // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE WS2812                                       // Using APA102, WS2812, WS2801. Don't forget to modify LEDS.addLeds to suit.
 #define NUM_LEDS 22                                          // Number of LED's.

uint8_t max_bright = 255;                                     // Overall brightness.

struct CRGB leds[NUM_LEDS];                                   // Initialize our LED array.

CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending = LINEARBLEND;                  // NOBLEND or LINEARBLEND


const uint8_t GROUPID = 0;                                    // Items 1 through 19
const uint8_t STRANDID = 1;                                   // Choose 1 to 99. Where 0 = all. 100 - 255 reserved for future use.

uint8_t mydevice = 0;                                         // This is our STRANDID
uint8_t mybri = 255;
uint8_t myhue = 0;
uint8_t myspeed = 40;


uint8_t   mypayload = 0;

// cool: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
uint8_t mycool = 40;


// spark: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
uint8_t myspark = 60;



void setup() {

  Serial.begin(115200);                                                                     // Initialize serial port for debugging.
  delay(1000);                                                                              // Soft startup to ease the flow of electrons.

  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);                              // Use this for WS2812

  FastLED.setBrightness(max_bright);
  set_max_power_in_volts_and_milliamps(5, 500);                                             // FastLED Power management set at 5V, 500mA.

  currentPalette  = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Orange, CRGB::Yellow);
  targetPalette  = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Orange, CRGB::Yellow);

} // setup()



void loop () {

  networking();                                                                             // We need to run this continually in case we disconnect from the MQTT broker.

  client.loop();                                                                            // Continuous check of MQTT.
  
  EVERY_N_MILLISECONDS(100) {                                                               // FastLED based non-blocking FIXED delay.
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);                  // Awesome palette blending capability.
  }

  EVERY_N_MILLIS_I(thisTimer,50) {                                                          // This only sets the Initial timer delay. To change this value, you need to use thisTimer.setPeriod(); You could also call it thatTimer and so on.
    thisTimer.setPeriod(myspeed);                                                           // Use that as our update timer value.
    Fire2012WithPalettexy();
  }

  FastLED.show();

} // loop()



void Fire2012WithPalettexy() {

  static byte heat[kMatrixWidth][kMatrixHeight];                                          // Array of temperature readings at each simulation cell

  for (int mw = 0; mw < kMatrixWidth; mw++) {                                             // Move along the width of the flame

  // Step 1.  Cool down every cell a little
    for (int mh = 0; mh < kMatrixHeight; mh++) {
      heat[mw][mh] = qsub8( heat[mw][mh],  random16(0, ((mycool * 10) / kMatrixHeight) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int mh = kMatrixHeight - 1; mh >= 2; mh--) {
      heat[mw][mh] = (heat[mw][mh - 1] + heat[mw][mh - 2] + heat[mw][mh - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8(0,255) < myspark ) {
      int mh = random8(3);
      heat[mw][mh] = qadd8( heat[mw][mh], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for (int mh = 0; mh < kMatrixHeight; mh++) {
      byte colorindex = scale8( heat[mw][mh], 240);
      leds[ XY(mh, mw)] = ColorFromPalette( targetPalette, colorindex, mybri, currentBlending);
    }
  } // for mw
  
} // Fire2012WithPalettexy()



uint16_t XY( uint8_t x, uint8_t y) {                                                      // x is height, y is width. Allows us to have 2 sided display.

  uint16_t i;
  
  if( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixHeight) + x;
  }

  if( kMatrixSerpentineLayout == true) {
    if( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixHeight - 1) - x;
      i = (y * kMatrixHeight) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * kMatrixHeight) + x;
    }
  }
  return i;
  
} // XY()



void callback(char *topic, byte *payload, unsigned int length) {

  Serial.print("Message arrived: ["); Serial.print(topic); Serial.println("]");           // Prints out anything that's arrived from broker and from the topic that we've subscribed to.

  int i; for (i = 0; i < length; i++) message_buff[i] = payload[i]; message_buff[i] = '\0'; // We copy payload to message_buff because we can't make a string out of payload.
  
  String msgString = String(message_buff);                                                // Finally, converting our payload to a string so we can compare it.

  mypayload = msgString.toInt();

  if (strcmp(topic, "fire/device")== 0) {                                                 // Device selector
    mydevice = msgString.toInt();
    Serial.print("device: "); Serial.println(mydevice);    
  }

  if (mydevice == 0 || mydevice == STRANDID) {

    if (strcmp(topic, "fire/bri") == 0) {                                                 // Brightness
      mybri = msgString.toInt();
      Serial.print("mybri: "); Serial.println(mybri);
    }
  
   if (strcmp(topic, "fire/hue")== 0) {                                                   // Hue
      myhue = msgString.toInt();
      Serial.print("myhue: "); Serial.println(myhue);
      targetPalette = CRGBPalette16(CHSV(myhue, 255, 8), CHSV(myhue+8, 248, 32), CHSV(myhue+32, 224, 128), CHSV(myhue+64, 192, 255));
   }
  
   if (strcmp(topic, "fire/speed")== 0) {                                                 // Speed
      myspeed = msgString.toInt();
      Serial.print("myspeed: "); Serial.println(myspeed);
    }
  
    if (strcmp(topic, "fire/cool")== 0) {                                                 // Cooling
      mycool = msgString.toInt();
      Serial.print("mycool: "); Serial.println(mycool);
    }
  
    if (strcmp(topic, "fire/spark")== 0) {                                                // Sparking
      myspark = msgString.toInt();
      Serial.print("myspark: "); Serial.println(myspark);
    }
  } // if mydevice
  
} // callback()



void networking() {                                                                         // Asynchronous network connect routine. Nice and generic.

  if (wifistart == false) {
    WiFi.begin(ssid, password);                                                             // Initialize WiFi on the ESP8266
    wifistart = true;
  }

  if (WiFi.status() == WL_CONNECTED && mqttconn == false) {
    Serial.print("IP address:\t"); Serial.println(WiFi.localIP());   
    client.setServer(mqttServer, mqttPort);                                                 // Initialize MQTT service.
    client.setCallback(callback);                                                           // Define the callback service to check with the server.
    client.connect("ESP8266Client", mqttUser, mqttPassword );
    mqttconn = true;
  }

  if (client.connected() && wifimqtt == false) {                                            // Now that we're connected. Let's subscribe to some topics.
    Serial.println("MQTT connected.");  

//    client.publish("LED1", "Hello from ESP8266!");                                        // Sends topic and payload to the MQTT broker.

    for (int i = 0; i < (sizeof(subscribetopic)/sizeof(int)); i++) {                           // Subscribes to topics from the MQTT broker.
      String mypref =  prefixtopic;
      mypref.concat(subscribetopic[i]);                                                        // Adds our prefix
      client.subscribe((char *) mypref.c_str());                                              // Concatenating strings is just getting ugly. Thank god for Stack Overflow, etc.
      Serial.println(mypref);      
    }
    wifimqtt = true;
  } // if client.connected()

  if (!client.connected() & wifimqtt == true) {                                             // We've lost connectivity to the broker, so let's reconnect.
    mqttconn = false;                                                                       // We've lost connectivity, so let's reset our flags back to 'not connected'.
    wifimqtt = false;
  }

} // networking()
