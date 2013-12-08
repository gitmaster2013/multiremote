#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <IRremote.h>
#include <String.h>


const unsigned int IR_RECEIVER_PIN = 9;
const unsigned int BAUD_RATE = 19200;

IRrecv ir_receiver(IR_RECEIVER_PIN);
decode_results results;

String txt;

// Update these with values suitable for your network.
byte mac[]    = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };

//MQTT definition
#define CLIENTID "Arduino"
byte server[] = { 192, 168, 3, 38 };


// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish("outTopic", p, length);
  // Free the memory
  free(p);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  ir_receiver.enableIRIn();
  Ethernet.begin(mac);
  Serial.print("server is at ") ;
    Serial.println(Ethernet.localIP()) ;
    int connRC = client.connect(CLIENTID) ;
    if (connRC)
    {
        client.publish("outTopic","Hi I am Arduino!");
        Serial.print("Arduino published a message to: ") ;
        Serial.println("outTopic") ;
        client.subscribe("inTopic") ;
        Serial.print("Arduino subscribed to: ") ;
        Serial.println("inTopic") ;
    }
    else
    {
        Serial.println(connRC) ;
    }
}

String dump(const decode_results* results)
{
        const int protocol = results->decode_type;
        String message;
        
	//Serial.print("Protocol: ");
	if (protocol == UNKNOWN)
	{
	Serial.println("not recognized.");
	}
	else
	{
		message +="CR/?0001?0001?";
                if (protocol == NEC)
		{
            	        message +="NEC";
		}
		else if (protocol == SONY)
		{
			message +="SONY";
		}
		else if (protocol == RC5)
		{
			message +="RC5";
		}
		else if (protocol == RC6)
		{
			message +="RC6";
		}
                message +="?";
                message += (results->bits, DEC);
                message +="?";
                message += (results->value, DEC);
	}
        Serial.println(message);
        return (message);
}

void loop()
{
	if (ir_receiver.decode(&results))
	{
                txt += dump(&results);
                ir_receiver.resume();
                char ssid[txt.length()];
                txt.toCharArray(ssid, txt.length());
                client.publish("outTopic",ssid);
                Serial.println(txt);
                txt = ""; 
        }
        client.loop();
}

