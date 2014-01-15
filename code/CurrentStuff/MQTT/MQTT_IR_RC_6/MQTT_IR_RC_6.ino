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
#define USER 1
#if USER == 1 // Matthias
#	define MQTTSERVER "david.dyndns.ultrachaos.de"
#	define CLIENTID "MLArduino"
#	define IRTOPIC "soll/matthias/ardunio/ir/#"
#	define RCTOPIC "soll/matthias/ardunio/rc/#"
#	define STATUSTOPIC "ist/matthias/ardunio/status"
#	define LASTTOPIC "ist/matthias/ardunio/last"
#	define WILLTOPIC "ist/matthias/ardunio/status"
#endif
#if USER == 2 // David
#	define MQTTSERVER "david.dyndns.ultrachaos.de"
#	define CLIENTID "DArduino"
#	define IRTOPIC "soll/wohnzimmer/ardunio/ir/#"
#	define RCTOPIC "soll/wohnzimmer/ardunio/rc/#"
#	define STATUSTOPIC "ist/wohnzimmer/ardunio/status"
#	define LASTTOPIC "ist/wohnzimmer/ardunio/last"
#	define WILLTOPIC "ist/wohnzimmer/ardunio/status"
#endif
#define WILLQOS 2
#define WILLRETAIN 1
#define WILLMESSAGE "off"
// SERIAL-DEFINES
#define BAUDRATE 19200

//** GLOBALS
// Ethernet
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAF };
byte ip[] = {192, 168, 178, 46};

//** INSTANCES
// Open IR-instances
IRrecv ir_receiver(IRRECEIVERPIN);
decode_results results;
IRsend irsender;
// Open RC-instance
RCSwitch RC = RCSwitch();
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
  client.publish(LASTTOPIC, p, length, HIGH);
  handle_command(topic,p,length);
  // Free the memory
  free(p);
}

void handle_command(char* topic, byte* bCmd, int length)
{
  String sCmd;
  String sLDR;
  String sTopic(topic);
  Serial.println("Topic is: ");
  Serial.print(topic);
  // Convert Byte to String-Array
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
        
	// Select topics and handles
  if (sTopic.endsWith("/ir"))
  {
    const int iParameter1 = atoi(args[0]);
  	const int iParameter2 = atoi(args[1]);
  	const long lParameter3 = atol(args[2]);
  	const int iParameter5 = atoi(args[4]);
  	handle_ir(iParameter1, iParameter2, lParameter3, iParameter5);
  }
  else if (sTopic.endsWith("/rc"))
  {
  	/* RC-INPUTS: 
  		TYP, FAMILY, GROUP, DEVICE, STATE */
  	handle_rc(args[0], args[1], args[2], args[3], args[4]);
  }
  else Serial.println("Topic unknown!");
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

//** function to handle RC-Commands
// Input: char-pointer with
// typ, family-code, group-code, device, state
// typ must be 1,2,3 or 4
// state must be 0 or 1
//** family can be ignaored if not typ 3 
void handle_rc(char* cTyp, char* cFamily, char* cGroup, char* cDevice, char* cState)
{
// V.0.4.140114ML

	// Debug-Info
  Serial.print("RC-Befehl: Protocol: ");
  Serial.print(cTyp);
  Serial.print(" Family: ");
  Serial.print(cFamily);
  Serial.print(", Group: ");
  Serial.print(cGroup);
  Serial.print(", Device: ");
  Serial.print(cDevice);
  Serial.print(", State: ");
  Serial.println(cState);
  
  // Typcasting and Definitions  
  const int iTyp = atoi(cTyp);
  const int iState = atoi(cState);
  
  // check for protocoll
  switch(iTyp)
  {
  case 1:  //TypeA_WithDIPSwitches
    {
      // first 5 DIP switches. Example ON-ON-OFF-OFF-ON.
      // last 5 DIP switches. Example OFF-ON-OFF-ON-OFF. 
      // 
      RC.setPulseLength(320);
      if (iState == 0)
      {
        RC.switchOff(cGroup, cDevice);
      }
      else if (iState == 1)
      {
        RC.switchOn(cGroup, cDevice);
      }
      else Serial.println("Protokoll nicht gefunden - TYP 1");
    }
	break;
  case 2:  //TypeB_WithRotaryOrSlidingSwitches
    {
      // first rotary switch. Example "1" or "A" or "I". 
      // second rotary switch. Example "4" or "D" or "IV".
      // 
      int iGroup = atoi(cGroup);
      int iDevice = atoi(cDevice);
      RC.setPulseLength(320);
      if (iState == 0)
      {
        RC.switchOff(iGroup, iDevice);
      }
      else if (iState == 1)
      {
        RC.switchOn(iGroup, iDevice);
      }
      else Serial.println("Protokoll nicht gefunden - TYP 2");
    }
    break;
//******
    	// To be tested
//*****
  case 3:  //TypeC_Intertechno
    {
      // first parameter familycode (a, b, c, ... f)
      // second parameter group number
      // third parameter device number
      // For example it's family 'b', group #3, device #2
      // 
      int iGroup = atoi(cGroup);
      int iDevice = atoi(cDevice);
      RC.setPulseLength(320);
      if (iState == 0)
      {
        RC.switchOff(*cFamily, iGroup, iDevice);
      }
      else if (iState == 1)
      {
        RC.switchOn(*cFamily, iGroup, iDevice);
      }
      else Serial.println("Protokoll nicht gefunden - TYP 3");
    }
    break;
  case 4:  //TypeD_REV
    {
      // first parameter group (a, b, c, d)
      // second parameter device number
      // For example it's family 'd', device #2
      //
      int iDevice = atoi(cDevice);
      RC.setPulseLength(360);
      if (iState == 0)
      {
        RC.switchOff(cGroup, iDevice);
      }
      else if (iState == 1)
      {
        RC.switchOn(cGroup, iDevice);
      }
      else Serial.println("Protokoll nicht gefunden - TYP 4");
    }
    break;
  case 5:  //RAW
    {
      //RC.setPulseLength(360);
    }
    break;
  default:
    {
      Serial.println("Protokoll nicht gefunden");
    }
    break;
  }
}

// SETUP
void startmqtt()
{
  // initialize mqttclient
  if (client.connect(CLIENTID, WILLTOPIC, WILLQOS, WILLRETAIN, WILLMESSAGE))
  {
    client.publish(STATUSTOPIC, "on");
    Serial.print("Arduino published a message to: ") ;
    Serial.println(STATUSTOPIC) ;
    client.subscribe(IRTOPIC);
    client.subscribe(RCTOPIC);
    Serial.print("Arduino subscribed to: ") ;
    Serial.print(IRTOPIC) ;
    Serial.print(" AND ") ;
    Serial.println(RCTOPIC) ;
  }
}

// SETUP
void setup()
{
	Serial.println("Try to connect to MQTT....");
  // initialize the LED pin as an output:
  pinMode(LEDPIN, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(BUTTONPIN, INPUT);   
  // Init Serial
  Serial.begin(BAUDRATE);
  // Init IR
  ir_receiver.enableIRIn();
  // Init RC
  RC.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2
  RC.enableTransmit(8); // Transmitterpindef.
  // Init Ethernet
  Serial.println("Try to get online....");
  Ethernet.begin(mac, ip);
  Serial.print("Arduino is at ") ;
  Serial.println(Ethernet.localIP()) ;
  // Init MQTT
  startmqtt();
}

void loop()
{
  if (!client.loop())
  {
    startmqtt(); 
  }
}











