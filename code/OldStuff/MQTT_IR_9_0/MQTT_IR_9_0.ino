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
#define LDRPIN A0 			//analog pin 0
#define IRRECEIVERPIN 9
// MQTT-DEFINES
#define MQTTSERVER "loki"
#define CLIENTID "MDArduino"
#define INTOPIC "soll/schlafzimmer-david/arduino/#"
#define OUTTOPIC "MDoutTopic"
// SERIAL-DEFINES
#define BAUDRATE 19200

//** GLOBALS
// Ethernet
byte mac[]    = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };	// Update these with values suitable for your network.
// used vars
long previousMillis = 0;
bool bLightState = LOW;
bool bButtonState = LOW;
int LDRReading;
String txt;

//** INSTANCES
// Open IR-instances
IRrecv ir_receiver(IRRECEIVERPIN);
decode_results results;
IRsend irsender;
// Open ethernet-instance
EthernetClient ethClient;
// Open MQTT-instance
void callback(char* topic, byte* payload, unsigned int length);	// Callback function header
PubSubClient client(MQTTSERVER, 1883, callback, ethClient);

//** FUNCTIONS
// Callback function
void callback(char* topic, byte* payload, unsigned int length)
{
    // In order to republish this payload, a copy must be made
    // as the orignal payload buffer will be overwritten whilst
    // constructing the PUBLISH packet.

    // Allocate the correct amount of memory for the payload copy
    byte* p = (byte*)malloc(length);
    // Copy the payload to the new buffer
    memcpy(p,payload,length);
    client.publish(OUTTOPIC, p, length);
    handle_command(p,length);
    // Free the memory
    free(p);
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
        message += "CR/?0001?0001?";
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
        message +="-";
        message += results->bits;
        message +="-";
        message += results->value;
    }
    Serial.println(message);
    return (message);
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
    // LDR
    if(sCmd=="CE/LDR")
    {
        int LDRReading = analogRead(LDRPIN);
        Serial.print("LDR: ");
        Serial.println(LDRReading);
        sLDR = "LDR: ";
        sLDR += String(LDRReading);
        char cLDR[sLDR.length()];
        sLDR.toCharArray(cLDR, sLDR.length());
        client.publish(OUTTOPIC, cLDR);
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
    }
    break;
    default:
    {
        Serial.println("Befehl nicht gefunden");
    }
    break;
    }
    bButtonState = digitalRead(BUTTONPIN);         // variable for reading the pushbutton status
    if (bButtonState == HIGH)
    {     
        // turn LED on:    
        digitalWrite(LEDPIN, HIGH);  
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
    Ethernet.begin(mac);
    Serial.print("server is at ") ;
    Serial.println(Ethernet.localIP()) ;
    int connRC = client.connect(CLIENTID) ;
    if (connRC)
    {
        client.publish(OUTTOPIC,"Hi I am the new Arduino!");
        Serial.print("Arduino published a message to: ") ;
        Serial.println(OUTTOPIC) ;
        client.subscribe(INTOPIC) ;
        Serial.print("Arduino subscribed to: ") ;
        Serial.println(INTOPIC) ;
    }
    else
    {
        Serial.println(connRC) ;
    }
}

// MAINLOOP
void loop()
{
    //LDR
    unsigned long currentMillis = millis();
    LDRReading = analogRead(LDRPIN);
    String sLDR;
    //  if(currentMillis - previousMillis > 5000)
    //  {
    // save the last time LDR been updated
    //previousMillis = currentMillis;
    if(LDRReading >= 70 && bLightState == LOW)
    {
        //sLDR = "Light: ON (";
        bLightState = HIGH;
        sLDR = "Light: ON (";
        delay(100);
        LDRReading = analogRead(LDRPIN);
        sLDR += String(LDRReading);
        sLDR += ")";
        char cLDR[sLDR.length()+1];
        sLDR.toCharArray(cLDR, sLDR.length()+1);
        client.publish(OUTTOPIC, cLDR);
        client.publish("MD/LIGHT", cLDR);
        sLDR ="";
    }
    if(LDRReading <= 70 && bLightState == HIGH)
    {
        //sLDR = "Light: OFF (";
        bLightState = LOW;
        sLDR = "Light: OFF (";
        delay(100);
        LDRReading = analogRead(LDRPIN);
        sLDR += String(LDRReading);
        sLDR += ")";
        char cLDR[sLDR.length()+1];
        sLDR.toCharArray(cLDR, sLDR.length()+1);
        client.publish(OUTTOPIC, cLDR);
        client.publish("MD/LIGHT", cLDR);
        sLDR ="";
    }

    if (ir_receiver.decode(&results))
    {
        txt += dump(&results);
        ir_receiver.resume();
        char ssid[txt.length()];
        txt.toCharArray(ssid, txt.length());
        client.publish(OUTTOPIC,ssid);
        Serial.println(txt);
        txt = "";
    }
    client.loop();
    bButtonState = digitalRead(BUTTONPIN);
    if (bButtonState == HIGH)
    {     
    // turn LED on:    
    digitalWrite(LEDPIN, HIGH);  
    }
    else {
    // turn LED off:
    digitalWrite(LEDPIN, LOW); 
    }
}

