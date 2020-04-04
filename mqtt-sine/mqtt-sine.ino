/* mqtt-sine
 *
 * By: Andrew Tuline
 *
 * Date: January, 2017
 *
 * Update: July, 2019   (MQTT Update)
 *
 * What can you do with a sine wave? LOTS!!
 * 
 * Forget using delay() statements or counting pixels up and down the strand of LED's. That's for chumps.
 * This example demonstrates just a bit of what can be done using basic high school math by using a simple sinewave.
 *
 * You can use a sine wave to go back and forth up a strand of LED's. You can shift the phase of a sine wave to move along a strand. You can clip the top of that sine wave
 * to just display a certain value (or greater). You can add palettes to that sine wave so that you aren't stuck with CHSV values and so much more.
 *
 * It shows that you don't neeed a pile of for loops, delays or counting pixels in order to come up with a decent animation. The display routine is SHORT. 
 *  
 * Oh, and look. It now contains MQTT functionality so that you can control a pile of variables with your phone. You can even support multiple devices.
 *
 *
 * MQTT Topics/Controls include:
 *
 * Widget         Topic             Payload
 * Combo Box      sine/device       0 = ALL Devices or 1-9
 * Slider         sine/bri          0 - 255
 * Slider         sine/hue          0 - 255
 * Slider         sine/speed        -5 - 5
 * Slider         sine/rot          16
 * Slider         sine/cent         0 - NUM_LEDS
 * 
 *
 * Note: You can configure a topic prefix of "sine/".
 * 
 * The Android phone application used is called 'IoT MQTT Panel Pro'.
 *
 */


// Use qsuba for smooth pixel colouring and qsubd for non-smooth pixel colouring
#define qsubd(x, b)  ((x>b)?b:0)                   // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)                          // Analog Unsigned subtraction macro. if result <0, then => 0

#include <ESP8266WiFi.h>                                    // Should be included when you install the ESP8266.
#include <PubSubClient.h>                                   // https://github.com/knolleary/pubsubclient
#include "FastLED.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif


// WiFi Authentication -------------------------------------------------------------------------
const char* ssid = "SM-G965W4004";                            // WiFi configuration for Android tethering network. The Android IP address is hard coded at 192.168.43.1.
const char* password = "afng2036";


// MQTT Authentication -------------------------------------------------------------------------

// My Phone as a tethered device (working)                  // MQTT Broker application on Android credentials - using the Android tethered WiFi.
const char* mqttServer = "192.168.43.1";
const int mqttPort = 1883;
const char* mqttUser = "wmabzsy";
const char* mqttPassword = "GT8Do3vkgWP5";
const char* mqttID ="12";                                       // Must be UNIQUE for EVERY display!!!

const uint8_t STRANDID = 12;                                   // Choose 1 to 99. Where 0 = all. 100 - 255 reserved for future use.


const char *prefixtopic = "sine/";
const char *subscribetopic[] = {"device", "bri", "hue", "speed", "rot", "cent"};

char message_buff[100];

WiFiClient espClient;
PubSubClient client(espClient);


uint8_t mydevice = 0;                                         // This is our STRANDID

uint8_t   mypayload = 0;                                      // MQTT message received.



// Fixed definitions cannot change on the fly.
#define LED_DT D5                                             // Serial data pin for WS2812 or WS2801.
#define LED_CK 11                                             // Serial clock pin for WS2801 or APA102.
#define COLOR_ORDER GRB                                       // Are they GRB for WS2812 and GBR for APA102
#define LED_TYPE WS2812                                       // What kind of strip are you using? WS2812, APA102. . .
#define NUM_LEDS 40                                           // Number of LED's.

uint8_t max_bright = 128;                                     // Overall brightness definition. It can be changed on the fly.

struct CRGB leds[NUM_LEDS];                                   // Initialize our LED array.

// Initialize changeable global variables. Play around with these!!!
int8_t thisspeed = 2;                                         // You can change the speed of the wave, and use negative values.
int thisphase = 0;                                            // Phase change value gets calculated.
int thisdelay = 20;                                           // You can change the delay. Also you can change the allspeed variable above.
uint8_t thisrot = 16;
const uint8_t phasediv = 4;                                   // A divider so that the speed has a decent range.
uint8_t thiscenter = NUM_LEDS / 2;

// Palette definitions
CRGBPalette16 currentPalette;
TBlendType currentBlending = LINEARBLEND;



void setup() {

  Serial.begin(115200);                                       // Initialize serial port for debugging.
  delay(1000);                                                // Soft startup to ease the flow of electrons.

  //  LEDS.addLeds<LED_TYPE, LED_DT, LED_CK, COLOR_ORDER>(leds, NUM_LEDS);   //WS2801 and APA102
  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);  // WS2812

  FastLED.setBrightness(max_bright);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);               // FastLED Power management set at 5V, 500mA.

  currentPalette = LavaColors_p;

  client.setServer(mqttServer, mqttPort);                                                 // Initialize MQTT service. Just once is required.
  client.setCallback(callback);                                                           // Define the callback service to check with the server. Just once is required.
  
} // setup()



void loop () {

  networking();                                               // We need to run this continually in case we disconnect from the MQTT broker.

  EVERY_N_MILLISECONDS(thisdelay) {                           // FastLED based non-blocking delay to update/display the sequence.
    one_sine_phase();                                         // The value is used as a color index.
  }

  FastLED.show();

} // loop()



void one_sine_phase() {                                       // This is the heart of this program. Sure is short.

  uint8_t thisindex = millis() / thisrot;
  thisphase += thisspeed;                                     // You can change direction and speed individually.

  for (int k = 0; k < NUM_LEDS - 1; k++) {                    // For each of the LED's in the strand, set a brightness based on a wave as follows:

//    int thisbright = cubicwave8((k * allfreq) + thisphase * k / phasediv);          // Brightness based on lots of variables.

    int thisbright = cubicwave8((thisphase * k) / phasediv);          // Brightness based on lots of variables.

    if ((thiscenter + k) <  NUM_LEDS) leds[(thiscenter + k) % NUM_LEDS] = ColorFromPalette( currentPalette, thisindex, thisbright, currentBlending);     // Let's now add the foreground colour with centering.
    if ((thiscenter - k) >= 0       ) leds[(thiscenter - k) % NUM_LEDS] = ColorFromPalette( currentPalette, thisindex, thisbright, currentBlending);     // Let's now add the foreground colour with centering.
    thisindex += 256 / NUM_LEDS;
    thisindex += thisrot;
        
  } // for k

} // one_sine_phase()



void callback(char *topic, byte *payload, unsigned int length) {                          // MQTT Interrupt routine because we just got a message!

  Serial.print("Topic: ["); Serial.print(topic); Serial.println("]");           // Prints out anything that's arrived from broker and from the topics that we've subscribed to.

  int i; for (i = 0; i < length; i++) message_buff[i] = payload[i]; message_buff[i] = '\0'; // We copy payload to message_buff because we can't make a string out of byte payload.

  String msgString = String(message_buff);                                                // Finally, converting our payload to a string so we can compare it.
  Serial.print("Payload: ["); Serial.print(msgString); Serial.println("]");

  mypayload = msgString.toInt();

  if (strcmp(topic, "sine/device") == 0) {                                                     // Device selector
    mydevice = msgString.toInt();
    Serial.print("device: "); Serial.println(mydevice);
  }

//  if (mydevice == GROUPID || mydevice == STRANDID) {

    if (strcmp(topic, "sine/bri") == 0) {                                                         // Brightness
      max_bright = msgString.toInt();
      Serial.print("mybri: "); Serial.println(max_bright);
      FastLED.setBrightness(max_bright);          
    }

    if (strcmp(topic, "sine/hue") == 0) {                                                         // Hue
      uint8_t baseC = msgString.toInt();
      Serial.print("baseC: "); Serial.println(baseC);
      currentPalette = CRGBPalette16(CHSV(baseC + random8(128), 255, random8(128, 255)),
                                    CHSV(baseC + random8(64),  255, random8(128, 255)),
                                    CHSV(baseC + random8(128), 64, random8(128, 255)),
                                    CHSV(baseC + random8(32),  255, random8(128, 255)));
    }
    
     if (strcmp(topic, "sine/speed")== 0) {                                                       // Speed 1 to 10
        thisspeed = msgString.toInt();
        Serial.print("thisspeed: "); Serial.println(thisspeed);
     }

     if (strcmp(topic, "sine/rot")== 0) {
        thisrot = msgString.toInt();
        Serial.print("thisrot: "); Serial.println(thisrot);                                       // rot 0 - 255 (was 16)
     }

     if (strcmp(topic, "sine/cent")== 0) {
        thiscenter = msgString.toInt() % NUM_LEDS;
        Serial.print("thiscenter: "); Serial.println(thiscenter);                                 // thiscenter - NUM_LEDS/2
     }

//  } // if mydevice

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
