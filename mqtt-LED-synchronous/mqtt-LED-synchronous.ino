/*
 * File: mqtt-LED1-synchronous.ino
 * 
 * By: Andrew Tuline
 * 
 * Date: June, 2019
 * 
 * This introduction to MQTT messaging blinks the internal LED of an ESP8266 based WeMOS D1 Mini.
 * 
 * The Android client is an application called 'IoT MQTT Panel'. There's a Pro version which has a few more features and no advertising.
 * 
 * 
 * Tested working configurations include:
 * 
 * 1) ESP8266 running PubSubclient DHCP'ed to my home network, cloudMQTT.com as the broker and IoT MQTT Panel as the publisher/client on my Android.
 * 2) ESP8266 running PubSubClient DHCP'ed to my home network, MQTT Broker App on my Android and IoT MQTT Panel as the publisher/client on my Android.
 * 3) ESP8266 running PubSubClient DHCP'ed to my home network, Mosquitto running on my Windows workstation and IoT MQTT Panel as the publisher/client on my Android. Only works when I'm at home.
 * 4) ESP8266 running PubSubClient DHCP'ed to my tethered Android (as 192.168.43.1), MQTT Broker App on my Android and IoT MQTT Panel as the publisher/client on my Android.
 * 
 * 
 * Information  Value(s)  Type
 * ===========  =======   ====
 * MQTT Topic   LED1      String (or char*)
 *    Payload   0, 1      Byte, converted to string, then int              // The Control Panel outputs ASCII strings as the payload. We'll convert that to an integer.
 * 
 * 
 * To run this:
 * 
 * 1) Configure and enable tethering on your Android phone with the credentials shown below.
 * 2) Install configure and start an MQTT broker (i.e. MQTT Broker App) on your Android phone and configure with credentials shown below.
 * 3) Alternatively setup an MQTT broker on the Internet, i.e. cloudMQTT.com or on your workstation/laptop.
 * 4) Install an MQTT client (i.e. IoT MQTT Panel) on your Android phone and configure with the configuration and credentials shown below.
 * 5) Setup a device and panel to communicate between Control Panel <---> MQTT Broker <---> ESP8266
 * 
 * 
 * For more information, see: http://tuline.com/getting-started-with-esp8266-and-mqtt/
 * 
 */


#include <ESP8266WiFi.h>                                    // Should be included when you install the ESP8266.
#include <PubSubClient.h>                                   // https://github.com/knolleary/pubsubclient


// WiFi Authentication -------------------------------------------------------------------------

const char* ssid = "SM-G965W4004";                        // WiFi configuration for Android tethering network. The Android IP address is hard coded at 192.168.43.1.
const char* password = "afng2036";


// MQTT Authentication -------------------------------------------------------------------------

// My Phone as a tethered device (working)                  // MQTT Broker application on Android credentials, using the Android tethered WiFi.
const char* mqttServer = "192.168.43.1";
const int mqttPort = 1883;
const char* mqttUser = "wmabzsy";
const char* mqttPassword = "GT8Do3vkgWP5";

char message_buff[100];
 
WiFiClient espClient;
PubSubClient client(espClient);



void setup() {
 
  Serial.begin(115200);

  pinMode(BUILTIN_LED, OUTPUT);
 
  WiFi.begin(ssid, password);                                                               // Initialize WiFi on the ESP8266
 
  while (WiFi.status() != WL_CONNECTED) {                                                   // Test to see if connected and wait until done.
    delay(500);
    Serial.println("Connecting to WiFi..");
  }                                                                                         // Can't proceed until connected to WiFi.
  
  Serial.println("Connected to the WiFi network.");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());                                                           // Optionally displays our IP address.
 
  client.setServer(mqttServer, mqttPort);                                                   // Initialize MQTT service.
  client.setCallback(callback);                                                             // Define the callback service to check with the server.
 
  while (!client.connected()) {                                                             // We will eventually want to move this so that if we lose connection, we can re-run this.
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {                         // Connect to the server with credentials and test for that.
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
     }
  }                                                                                         // Can't proceed until connected to MQTT Broker.
 
  client.publish("LED1", "Hello from ESP8266!");                                            // Sends topic and payload to the MQTT broker, just for fun.

  client.subscribe("LED1");                                                                 // Subscribes to topic from the MQTT broker. This ione is IMPORTANT!
  client.subscribe("LED2");																	                                // We can subscribe to more than 1 topic. This one is not used here.
 
} // setup()


  
void loop() {

  client.loop();                                                                            // You need to run this in order to allow the client to process incoming messsages and to publish the message.

} // loop()



void callback(char *topic, byte *payload, unsigned int length) {
  
  Serial.print("Message arrived: ["); Serial.print(topic); Serial.println("]");             // Prints out any topic that has arrived and is a topic that we subscribed to.

  int i; for (i = 0; i < length; i++) message_buff[i] = payload[i]; message_buff[i] = '\0';   // We copy payload to message_buff because we can't make a string out of a byte based payload.
  
  String msgString = String(message_buff);                                                  // Converting our payload to a string so we can compare it.
  uint8_t msgVal = msgString.toInt();                                                       // Now, let's convert that payload to an integer for numeric comparisons below.
                                                                                            // If we just used strings as the payload, then we don't need to convert to int.
                                                                                            // Also, if we use JSON data as the payload, then we need to add that functionality as well.
  Serial.print("Message: ");  Serial.println(msgVal);
  if (strcmp(topic, "LED1") == 0) {                                                         // Returns 0 if the strings are equal, so we have received our topic.
    if (msgVal == 1) {                                                                      // Payload of our topic is 1, so we'll turn on the LED.
      digitalWrite(BUILTIN_LED, LOW);                                                       // PIN LOW will turn off the LED.
    }
    if (msgVal == 0) {                                                                      // Payload of our topic is 0, so we'll turn off the LED.
      digitalWrite(BUILTIN_LED, HIGH);                                                      // PIN HIGH will turn on the LED.
    }
  }
} // callback()
