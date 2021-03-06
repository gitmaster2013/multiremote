#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <IRremote.h>
#include <String.h>


const unsigned int IR_RECEIVER_PIN = 9;
const unsigned int BAUD_RATE = 19200;

IRrecv ir_receiver(IR_RECEIVER_PIN);
decode_results results;

IRsend irsender;

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
  handle_command(p,length);
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
                message += results->bits;
                message +="?";
                message += results->value;
	}
        Serial.println(message);
        return (message);
}

void handle_command(byte* bCmd, int length)
{
    String sCmd;
    // (5 gibt die Anzahl der Parameter an)
    Serial.print("COMMAND: ");
    for (int i=0; i < length; i++)
    {
       sCmd += char(bCmd[i]) ;
       Serial.print(char(bCmd[i]));
    }
    Serial.println();
    Serial.println(sCmd);
    char cCmd[sCmd.length()+1];
    sCmd.toCharArray(cCmd, sCmd.length()+1);
    Serial.println(cCmd);
    char* line = cCmd;
    strsep(&line, "/"); //Pointer von "/"
    char* path = strsep(&line, "/");  // Beginn von path auf das erste "/"
    // Das erste Zeichen des zu parsenden Strings ist das nach dem entfernten "/" 
    
    char* args[5];
    // (5 gibt die Anzahl der Parameter an)
    for (char** ap = args; (*ap = strsep(&path, "?")) != NULL;)
        if (**ap != '\0')
            // Bis der Uebergabestring zu ende ist...
            if (++ap >= &args[5])
                // ... werden alle mit "?" getrennten Teilstrings als CharArrays gespeichert (5 gibt die Anzahl der Parameter an)
                break;
    const int device = atoi(args[0]);
    // Das device-Char wird in ein i_Zahl umgewandelt
    const int typ = atoi(args[1]);
    // Das device-Char wird in ein i_Zahl umgewandelt
    char* protocol = "NEC";
    const int bits = atoi(args[3]);
    // Das bits-Char wird in ein i_Zahl umgewandelt
    const long value = atol(args[4]);
    // Das value-Char wird in ein f_Zahl umgewandelt
    Serial.print("Befehl: Device: ");
    Serial.print(device);
    Serial.print(", Typ: ");
    Serial.print(typ);
    Serial.print(", Protocol: ");
    Serial.print(args[2]);
    Serial.print(", Bits: ");
    Serial.print(bits);
    Serial.print(", Value: ");
    Serial.println(value);
    // DEBUG: Alle Parameter werden auf der Konsole ausgegeben
    switch(typ)
    // Unterscheidung der Protokolle
    {
    case 1:
    {
        Serial.print("Befehl = SENDE IR");
        Serial.print(" --> Protocol: ");
        Serial.print(args[2]);
        Serial.print(", Bits: ");
        Serial.print(bits);
        Serial.print(", Value: ");
        Serial.println(value);
        Serial.println();
        irsender.sendNEC(value, bits);
        // DEBUG: Alle IR-Sende-Parameter werden auf der Konsole ausgegeben
    }
    break;
    case 2:
    {
        Serial.println("Befehl = SENDE RF");
    }
    break;
    default:
    {
        Serial.println("Befehl nicht gefunden");
    }
    break;
    }
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

