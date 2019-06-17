/*
 * File: mqtt-demo.ino
 * 
 * By: Andrew Tuline
 * 
 * This program demonstrates using MQTT messaging to blink the internal LED of an ESP8266 based WeMOS D1 Mini with JSON support.
 * 
 * The Android client is an application called 'IoT MQTT Panel'. There's a Pro version which I ended up buying.
 * 
 * 
 * 
 * Tested working configurations include:
 * 
 * 1) ESP8266 running PubSubclient DHCP'ed to my home network, cloudMQTT.com as the broker and IoT MQTT Panel as the publisher/client on my Android.
 * 2) ESP8266 running PubSubClient DHCP'ed to my home network, MQTT Broker App on my Android and IoT MQTT Panel as the publisher/client on my Android.
 * 3) ESP8266 running PubSubClient DHCP'ed to my home network, Mosquitto running on my Windows workstation and IoT MQTT Panel as the publisher/client on my Android. Only works when I'm at home.
 * 4) ESP8266 running PubSubClient DHCP'ed to my tethered Android (as 192.168.43.1), MQTT Broker App on my Android and IoT MQTT Panel as the publisher/client on my Android.
 * 
 */


#include <ESP8266WiFi.h>                                      // Should be included when you install the ESP8266.
#include <PubSubClient.h>                                     // https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>                                      // https://github.com/bblanchon/ArduinoJson


// WiFi Authentication -------------------------------------------------------------------------
const char* ssid = "SM-G965W4004";                            // WiFi configuration for Android tethering network. The Android IP address is hard coded at 192.168.43.1.
const char* password = "afng2036";


// MQTT Authentication -------------------------------------------------------------------------

// My Phone as a tethered device (working)                    // MQTT Broker application on Android credentials - using the Android tethered WiFi.
const char* mqttServer = "192.168.43.1";
const int mqttPort = 1883;
const char* mqttUser = "wmabzsy";
const char* mqttPassword = "GT8Do3vkgWP5";

const char *prefixtopic = "JSON/";
const char *subscribetopic[] = {"LED1", "JSON1"};             // Topics that we will subscribe to.

char message_buff[100];
 
WiFiClient espClient;
PubSubClient client(espClient);

bool wifistart = false;                                       // State variables for asynchronous wifi & mqtt connection.
bool mqttconn = false;
bool wifimqtt = false;



void setup() {
 
  Serial.begin(115200);

  pinMode(BUILTIN_LED, OUTPUT);
 
} // setup()


  
void loop() {

  networking();
  
  client.loop();                                                                            // You need to run this in order to allow the client to process incoming messsages, to publish message and to refresh the connection.

} // loop()



void callback(char *topic, byte *payload, unsigned int length) {

  StaticJsonDocument<256> doc;                                                              // Allocate memory for the doc array.
  deserializeJson(doc, payload, length);                                                    // Convert JSON string to something useable.

  Serial.print("Message arrived: ["); Serial.print(topic); Serial.println("]");             // Prints out anything that's arrived from broker and from the topic that we've subscribed to.

  int i; for (i = 0; i < length; i++) message_buff[i] = payload[i]; message_buff[i] = '\0';  // We copy payload to message_buff because we can't make a string out of payload.
  
  String msgString = String(message_buff);                                                  // Finally, converting our payload to a string so we can compare it.

//  Serial.print("Message: ");  Serial.println(msgString);

  if (strcmp(topic, "JSON/LED1") == 0) {                                                    // Returns 0 if the strings are equal, so we have received our topic.
    if (msgString == "1") {                                                                 // Payload of our topic is '1', so we'll turn on the LED.
      digitalWrite(BUILTIN_LED, LOW);                                                       // PIN HIGH will switch ON the relay
    }
    if (msgString == "0") {                                                                 // Payload of our topic is '0', so we'll turn off the LED.
      digitalWrite(BUILTIN_LED, HIGH);                                                      // PIN LOW will switch OFF the relay
    }
  } // if LED1

  if (strcmp(topic, "JSON/JSON1") == 0) {                                                    // Returns 0 if the strings are equal, so we have received our topic.
    const char *name = doc["name"];
    Serial.print("name: "); Serial.println(name);
    uint16_t age = doc["age"];
    Serial.print("age: "); Serial.println(age);
    const char *city = doc["city"];
    Serial.print("city: "); Serial.println(city);
  } // if JSON/JSON1
  
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
