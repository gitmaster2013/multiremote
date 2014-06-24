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
#define IRTOPIC "soll/wohnzimmer/ardunio/ir/#"
#define RCTOPIC "soll/wohnzimmer/ardunio/rc/#"
#define OUTTOPIC "ist/wohnzimmer/ardunio/status"
#define LASTTOPIC "ist/wohnzimmer/ardunio/last"
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
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish(LASTTOPIC, p, length);
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
  //strsep(&line, "/"); //Pointer von "/"
  //char* path = strsep(&line, "/");  // Beginn von path auf das erste "/"
  // Das erste Zeichen des zu parsenden Strings ist das nach dem entfernten "/"

  char* args[5];
  // (5 gibt die Anzahl der Parameter an)
  for (char** ap = args; (*ap = strsep(&line, "-")) != NULL;)
    if (**ap != '\0')
      // Bis der Uebergabestring zu ende ist...
      if (++ap >= &args[5])
        // ... werden alle mit "?" getrennten Teilstrings als CharArrays gespeichert (5 gibt die Anzahl der Parameter an)
        break;
  const int iParameter1 = atoi(args[0]);
  const int iParameter2 = atoi(args[1]);
  const long iParameter3 = atol(args[2]);
  const int iParameter4 = atoi(args[3]);
  const int iParameter5 = atoi(args[4]);
  if (iParameter4==1)
  {
    handle_ir(iParameter1, iParameter2, iParameter3, iParameter5);
    //handle_ir(int protocol, int bits, long value, int repeat)
  }
  else
  {
    handle_rc(iParameter1, iParameter2, iParameter3, iParameter5);
  }
}

void handle_ir(int protocol, int bits, long value, int repeat)
{
  Serial.print("IR-Befehl: Protocol: ");
  Serial.print(protocol);
  Serial.print(", Bits: ");
  Serial.print(bits);
  Serial.print(", Value: ");
  Serial.print(value);
  Serial.print(", Repeat: ");
  Serial.println(repeat);
  switch(protocol)
    // Unterscheidung der Protokolle
  {
  case 1:
    {
      irsender.sendNEC(value, bits);
    }
    break;
  case 2:
    {
      irsender.sendSony(value, bits);
    }
    break;
  case 3:
    {
      irsender.sendRC5(value, bits);
    }
    break;
  case 4:
    {
      irsender.sendRC6(value, bits);
    }
    break;
  case 5:
    {
      irsender.sendDISH(value, bits);
    }
    break;
  case 6:
    {
      irsender.sendSharp(value, bits);
    }
    break;
  case 7:
    {
      irsender.sendSamsung(value, bits);
    }
    break;
  case 8:
    {
      irsender.sendJVC(value, bits, repeat);
    }
    break;
  case 9:
    {
      //	  irsender.sendPanasonic(value, bits);
      //        irsender.sendPanasonic(unsigned long address, unsigned long data);
    }
    break;
  case 10:
    {
      //	  irsender.sendRaw(value, bits);
      //        irsender.sendRaw(unsigned int buf[], int len, int hz);
    }
    break;
  default:
    {
      Serial.println("Protokoll nicht gefunden");
    }
    break;
  }
}

void handle_rc(int protocol, int bits, long value, int repeat)
{
  Serial.print("IR-Befehl: Protocol: ");
  Serial.print(protocol);
  Serial.print(", Bits: ");
  Serial.print(bits);
  Serial.print(", Value: ");
  Serial.print(value);
  Serial.print(", Repeat: ");
  Serial.println(repeat);
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
    client.publish(OUTTOPIC,"on");
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





