#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <IRremote.h>
#include <String.h>

// set pin numbers:
const int buttonPin = 5;     // the number of the pushbutton pin
const int ledPin =  7;      // the number of the LED pin
int LDR_Pin = A0; //analog pin 0
long previousMillis = 0;
int iLightState = LOW;
int buttonState = 0;

const unsigned int IR_RECEIVER_PIN = 9;
const unsigned int BAUD_RATE = 19200;

IRrecv ir_receiver(IR_RECEIVER_PIN);
decode_results results;

IRsend irsender;

String txt;

// Update these with values suitable for your network.
byte mac[]    = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };

//MQTT definition
#define MQTTSERVER "david.dyndns.ultrachaos.de"
#define CLIENTID "MDArduino"
#define INTOPIC "MDinTopic"
#define OUTTOPIC "MDoutTopic"


// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(MQTTSERVER, 1883, callback, ethClient);

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

void setup()
{
    // initialize the LED pin as an output:
    pinMode(ledPin, OUTPUT);      
    // initialize the pushbutton pin as an input:
    pinMode(buttonPin, INPUT);   
    Serial.begin(BAUD_RATE);
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
        int LDRReading = analogRead(LDR_Pin);
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
    buttonState = digitalRead(buttonPin);         // variable for reading the pushbutton status
    if (buttonState == HIGH)
    {     
        // turn LED on:    
        digitalWrite(ledPin, HIGH);  
    }
}

void loop()
{
    //LDR
    unsigned long currentMillis = millis();
    int LDRReading = analogRead(LDR_Pin);
    String sLDR;
    //  if(currentMillis - previousMillis > 5000)
    //  {
    // save the last time LDR been updated
    //previousMillis = currentMillis;
    if(LDRReading >= 70 && iLightState == LOW)
    {
        //sLDR = "Light: ON (";
        iLightState = HIGH;
        sLDR = "Light: ON (";
        delay(100);
        LDRReading = analogRead(LDR_Pin);
        sLDR += String(LDRReading);
        sLDR += ")";
        char cLDR[sLDR.length()+1];
        sLDR.toCharArray(cLDR, sLDR.length()+1);
        client.publish(OUTTOPIC, cLDR);
        client.publish("MD/LIGHT", cLDR);
        sLDR ="";
    }
    if(LDRReading <= 70 && iLightState == HIGH)
    {
        //sLDR = "Light: OFF (";
        iLightState = LOW;
        sLDR = "Light: OFF (";
        delay(100);
        LDRReading = analogRead(LDR_Pin);
        sLDR += String(LDRReading);
        sLDR += ")";
        char cLDR[sLDR.length()+1];
        sLDR.toCharArray(cLDR, sLDR.length()+1);
        client.publish(OUTTOPIC, cLDR);
        client.publish("MD/LIGHT", cLDR);
        sLDR ="";
    }

    //}
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
    buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH)
    {     
    // turn LED on:    
    digitalWrite(ledPin, HIGH);  
    }
    else {
    // turn LED off:
    digitalWrite(ledPin, LOW); 
    }
}

