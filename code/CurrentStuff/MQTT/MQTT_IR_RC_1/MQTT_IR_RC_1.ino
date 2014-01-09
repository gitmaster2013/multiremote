//** INCLUDES
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <IRremote.h>
#include <String.h>
#include <RCSwitch.h>

//** DEFINES
// PIN-DEFINES
#define BUTTONPIN 5     	// the number of the pushbutton pin
#define LEDPIN 7      		// the number of the LED pin
#define LDRPIN A0 		//analog pin 0
#define IRRECEIVERPIN 9
#define RCSENDPIN 8
// MQTT-DEFINES
#define MQTTSERVER "loki"
#define CLIENTID "MDArduino"
#define IRTOPIC "soll/schlafzimmer-david/ardunio/ir/#"
#define RCTOPIC "soll/schlafzimmer-david/ardunio/rc/#"
#define OUTTOPIC "ist/schlafzimmer-david/ardunio/ir/outTopic"
// SERIAL-DEFINES
#define BAUDRATE 19200

//** GLOBALS
// Ethernet
byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };

//** INSTANCES
// Open IR-instances
IRrecv ir_receiver(IRRECEIVERPIN);
decode_results results;
IRsend irsender;
// Open RC-instance
RCSwitch Switch = RCSwitch();
// Open ethernet-instance
EthernetClient ethClient;
// Open MQTT-instance
void callback(char* topic, byte* payload, unsigned int length);	// Callback function header
PubSubClient client(MQTTSERVER, 1883, callback, ethClient);

//** FUNCTIONS
// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);

  for (int i=0; i < 60; i++)
  {
    Serial.print(char(topic[i]));
  }
  Serial.println();
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish(OUTTOPIC, p, length);
  handle_command(p,length);
  // Free the memory
  free(p);
}

void handle_command(byte* bCmd, int length)
{
  String sCmd;
  String sLDR;
  // (5 gibt die Anzahl der Parameter an)
  Serial.print("COMMAND: ");
  for (int i=0; i < length; i++)
  {
    sCmd += char(bCmd[i]) ;
  }
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
      Serial.print(" --> Protocol: ");
      Serial.print(args[2]);
      Serial.print(", Bits: ");
      Serial.print(bits);
      Serial.print(", Value: ");
      Serial.println(value);
      Serial.println();
      Switch.switchOn(bits, value);
    }
    break;
  default:
    {
      Serial.println("Befehl nicht gefunden");
    }
    break;
  }
}

// SETUP
void setup()
{
  // initialize the LED pin as an output:
  pinMode(LEDPIN, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(BUTTONPIN, INPUT);   
  Serial.begin(BAUDRATE);
  ir_receiver.enableIRIn();
  Switch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2
  Switch.enableTransmit(8); // Transmitterpindef.
  Switch.setPulseLength(260);
  Ethernet.begin(mac);
  Serial.print("server is at ") ;
  Serial.println(Ethernet.localIP()) ;
  if (client.connect(CLIENTID)) {
    client.publish(OUTTOPIC,"hello world");
    Serial.print("Arduino published a message to: ") ;
    Serial.println(OUTTOPIC) ;
    client.subscribe(IRTOPIC);
    client.subscribe(RCTOPIC);
    Serial.print("Arduino subscribed to: ") ;
    Serial.print(IRTOPIC) ;
    Serial.print(" AND ") ;
    Serial.println(RCTOPIC) ;
  }
}

void loop()
{
  client.loop();
}



