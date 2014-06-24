/*
 Publishing in the callback 
 
  - connects to an MQTT server
  - subscribes to the topic INTOPIC
  - when a message is received, republishes it to OUTTOPIC
  
  This example shows how to publish messages within the
  callback function. The callback function header needs to
  be declared before the PubSubClient constructor and the 
  actual callback defined afterwards.
  This ensures the client reference in the callback function
  is valid.
  
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// MQTT-DEFINES
#define MQTTSERVER "loki"
#define CLIENTID "MDArduino"
#define INTOPIC "soll/schlafzimmer-david/ardunio/#"
#define OUTTOPIC "ist/schlafzimmer-david/ardunio/ir/outTopic"




// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(MQTTSERVER, 1883, callback, ethClient);

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish(OUTTOPIC, p, length);
  // Free the memory
  free(p);
}

void setup()
{
  
  Ethernet.begin(mac);
  if (client.connect(CLIENTID)) {
    client.publish(OUTTOPIC,"hello world");
    client.subscribe(INTOPIC);
  }
}

void loop()
{
  client.loop();
}

