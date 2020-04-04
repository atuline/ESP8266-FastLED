/* File: mqtt-firexy.ino
 *  
 * Fire2012withPalette by: Mark Kriegsman
 * 
 * MQTT modifications by: Andrew Tuline
 *
 * Modified date: July, 2019
 *
 * Here's Mark Kriegsman's Fire2012withpalette configured with MQTT based messaging. This routine is different from the standard ethernet and MQTT programming 
 * in that it is non-blocking for both ethernet and MQTT broker connectivity. Surprisingly, making the MQTT component non-blocking was quite a challenge.
 * 
 * Setting: Down a winding ravine, I have lanterns spread along the banks of a river over the length of a football field. There is no power, wifi or data that I can rely on. 
 * My Android phone is configured as a mobile hotspot, as well as an MQTT broker and MQTT client that controls the the lanterns. Those ESP8266 based (and 18650 powered) lanterns
 * are configured as MQTT clients that receive commands from my Android phone. Those commands are translated to parameters that modify the fire2012 animation. Reception will be 
 * sporadic for the lanterns as I walk up and down the ravine, so we need to be able to connect/re-connect to the broker without affecting the animation of the lanterns.
 * 
 * 
 * Topic Prefix: fire/
 * 
 * Topic Function                   Widget      Topic   Value(s)  Description
 * --------------                   ------      -----   -------   ---------------------------
 * 
 * Device                           Combo box   device  0,1-9   Select All or a single device in the family.
 * Brightness                       Slider      bri     0-255   Decrease/Increase brightness.
 * Hue                              Slider      hue     0-255   Select palette base hue.
 * Speed                            Slider      speed   10-100  Change speed of animation.
 * Cooling                          Slider      cool    20-100  Change rate of cooling of flame.
 * Sparking                         Slider      spark   50-100  Change rate of sparking of new flames.                         
 * 
 * 
 * Sample Topic and Payload:     fire/bri    245
 * 
 *
 * The Android phone MQTT client is called 'IoT MQTT Panel Pro'.
 * The Android phone MQTT broker is called 'MQTT Broker App'.
 * 
 * Warning: The ESP8266 may require you to unplug and re-plug it in again after programming it before the wifi will work. 
 * 
 */


#include <ESP8266WiFi.h>                                        // Should be included when you install the ESP8266.
#include <PubSubClient.h>                                       // https://github.com/knolleary/pubsubclient
#include "FastLED.h"                                            // https://github.com/FastLED/FastLED

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif


// WiFi Authentication -------------------------------------------------------------------------
//const char* ssid = "WiFiPlus5041-2.4G";                       // WiFi configuration for home network. Default Gateway IP is 192.168.1.254
//const char* password =  "t84uw5hc29";

const char* ssid = "SM-G965W4004";                              // WiFi configuration for Android tethering network. The Android IP address is hard coded at 192.168.43.1.
const char* password = "afng2036";


// MQTT Authentication -------------------------------------------------------------------------

// My Phone as a tethered device                                // MQTT Broker application on Android credentials - using the Android tethered WiFi.
const char* mqttServer = "192.168.43.1";
const int mqttPort = 1883;
const char* mqttUser = "wmabzsy";
const char* mqttPassword = "GT8Do3vkgWP5";
const char* mqttID ="11";                                       // Must be numeric AND unique for EVERY display that is running at the same time and should match the int of STRANDID. 

// You can use this to serialize your device so that you can support more than 1 of these at the same time.
const int STRANDID = atoi(mqttID);                              // Choose 11 to 99. Where 0 = all. 1-9 reserved for future use.


const char *prefixtopic = "fire/";
const char* subscribetopic[] = {"device", "bri", "hue", "speed", "cool", "spark"};

char message_buff[100];
 
WiFiClient espClient;
PubSubClient client(espClient);


//IPAddress staticIP(192, 168, 43, 11);                         // ESP static ip.
//IPAddress gateway(192, 168, 43, 1);                           // IP Address of tethered phones.
//IPAddress subnet(255, 255, 255, 0);                           // Subnet mask.
//IPAddress dns(8, 8, 8, 8);                                    // DNS.

const uint8_t kMatrixWidth = 2;                               // Number of lengths.
const uint8_t kMatrixHeight = 11;                             // How long each length is.
const bool    kMatrixSerpentineLayout = true;                 // Using a serpentine layout.


#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// Fixed definitions cannot change on the fly.
#define LED_DT D5                                             // Data pin to connect to the strip.
#define COLOR_ORDER GRB                                       // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE WS2812                                       // Using APA102, WS2812, WS2801. Don't forget to modify LEDS.addLeds to suit.

uint8_t max_bright = 255;                                     // Overall brightness.

struct CRGB leds[NUM_LEDS];                                   // Initialize our LED array.

CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType    currentBlending = LINEARBLEND;                  // NOBLEND or LINEARBLEND



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
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);                                             // FastLED Power management set at 5V, 500mA.

  currentPalette  = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Orange, CRGB::Yellow);
  targetPalette  = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Orange, CRGB::Yellow);

  client.setServer(mqttServer, mqttPort);                                                 // Initialize MQTT service. Just once is required.
  client.setCallback(callback);                                                           // Define the callback service to check with the server. Just once is required.

} // setup()



void loop () {

  networking();                                                                             // We need to run this continually in case we disconnect from the MQTT broker.

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

  static byte heat[kMatrixWidth][kMatrixHeight];                                              // Array of temperature readings at each simulation cell

  for (int mw = 0; mw < kMatrixWidth; mw++) {                                                 // Move along the width of the flame

    for (int mh = 0; mh < kMatrixHeight; mh++) {                                              // Step 1.  Cool down every cell a little
      heat[mw][mh] = qsub8( heat[mw][mh],  random16(0, ((mycool * 10) / kMatrixHeight) + 2));
    }

    for (int mh = kMatrixHeight - 1; mh >= 2; mh--) {                                         // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      heat[mw][mh] = (heat[mw][mh - 1] + heat[mw][mh - 2] + heat[mw][mh - 2] ) / 3;
    }
    
    if (random8(0,255) < myspark ) {                                                          // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
      int mh = random8(3);
      heat[mw][mh] = qadd8( heat[mw][mh], random8(160,255) );
    }

    for (int mh = 0; mh < kMatrixHeight; mh++) {                                              // Step 4.  Map from heat cells to LED colors
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



void networking() {                                                                         // Asynchronous network connect routine with MQTT connect and re-connect. The trick is to make it re-connect when something breaks.

  static long lastReconnectAttempt = 0;
  static bool wifistart = false;                                                            // State variables for asynchronous wifi & mqtt connection.
  static bool wificonn = false;

  if (wifistart == false) {
    WiFi.begin(ssid, password);                                                             // Initialize WiFi on the ESP8266. We don't care about actual status.
    wifistart = true;
  }

  if (WiFi.status() == WL_CONNECTED && wificonn == false) {
    Serial.print("IP address:\t"); Serial.println(WiFi.localIP());                          // This should just print once.
    wificonn = true;    
  }

  if (WiFi.status() != WL_CONNECTED) wificonn = false;                                      // Toast the connection if we've lost it.

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {                               // Non-blocking re-connect to the broker. This was challenging.
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      if (reconnect()) {                                                                    // Attempt to reconnect.
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }

} // networking()



boolean reconnect() {                                                                       // Here is where we actually connect/re-connect to the MQTT broker and subscribe to our topics.
  
  if (client.connect(mqttID, mqttUser, mqttPassword )) {
    
    Serial.println("MQTT connected.");  
//    client.publish("LED1", "Hello from ESP8266!");                                        // Sends topic and payload to the MQTT broker.
    for (int i = 0; i < (sizeof(subscribetopic)/sizeof(int)); i++) {                        // Subscribes to list of topics from the MQTT broker. This whole loop is well above my pay grade.
      String mypref =  prefixtopic;                                                         // But first, we need to get our prefix.
      mypref.concat(subscribetopic[i]);                                                     // Concatenate prefix and the topic together with a little bit of pfm. 
      client.subscribe((char *) mypref.c_str());                                            // Now, let's subscribe to that concatenated and converted mess.
      Serial.println(mypref);                                                               // Let's print out each subscribed topic, just to be safe.
    }
  } // if client.connected()

  return client.connected();
  
} // reconnect()
