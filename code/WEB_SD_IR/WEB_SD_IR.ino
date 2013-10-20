// http://fluuux.de/2013/03/arduino-als-webserver-einrichten-und-webpage-von-sd-karte-laden/

#include <SPI.h>
#include <Ethernet.h>
#include <IRremote.h>
#include <TextFinder.h>
#include <SD.h>

// ### Voraussetzungen ###
// TSOP Signal-Pin <--> Arduino - Pin 11
// IR-LED Anode <--> Arduino - Pin 3
// Test-LED <--> Arduino - Pin 6

class AppleRemote
{
    enum
    {
      CMD_LEN = 32,
      UP = 0x77E1D01D,
      DOWN = 0x77E1B01D,
      PLAY = 0x77E1201D,
      PREV = 0x77E1101D,
      NEXT = 0x77E1E01D,
      MENU = 0x77E1401D
    };
    
  IRsend mac;
  
  void send_command(const long command) { 
    mac.sendNEC(command, CMD_LEN);
  }
    
    
    
    bool handle_command(char* line)
    {
        strsep(&line, " ");
        char* path = strsep(&line, " ");
        char* args[3];
        for (char** ap = args; (*ap = strsep(&path, "?")) != NULL;)
            if (**ap != '\0')
                if (++ap >= &args[3])
                    break;
        const int bits = atoi(args[1]);
        const long value = atol(args[2]);
        return send_ir_data(args[0], bits, value);
    }
    
public:
    bool send_ir_data(const char* protocol, const int bits, const long value)
    {
        bool result = true;
        if (!strcasecmp(protocol, "NEC"))
            mac.sendNEC(value, bits);
        else if (!strcasecmp(protocol, "SONY"))
            mac.sendSony(value, bits);
        else if (!strcasecmp(protocol, "RC5"))
            mac.sendRC5(value, bits);
        else if (!strcasecmp(protocol, "RC6"))
            mac.sendRC6(value, bits);
        else
            result = false;
        return result;
    }
    };

AppleRemote apple_remote;

const unsigned int PROXY_PORT = 80;
const unsigned int BAUD_RATE = 19200;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };
	// MAC Arduino Ethernet (David)
byte sdPin = 4;
	// Pin der SD-Karte
const int MAX_LINE = 256;
char line[MAX_LINE];

EthernetServer server(PROXY_PORT);
	// Server port

File webFile;

void setup()
{
    Serial.begin(BAUD_RATE);
    	// Open serial communications and wait for port to open:
    Ethernet.begin(mac);
    	// start the Ethernet connection and the server:
    Serial.print("Server is at: ");
    Serial.println(Ethernet.localIP());
    server.begin();
    	// Server starten
    Serial.println("ARDUINO - STEUERUNG");
    Serial.println("Initialisiere SD-Karte...");
    if (!SD.begin(sdPin))
    {
        Serial.println(" - Initialisierung der SD-Karte fehlgeschlagen!");
        return;
    }
    Serial.println(" - SD-Karte erfolgreich initialisiert.");

    if (!SD.exists("aprm2.htm"))
    {
        Serial.println(" - Datei (aprm2.htm) wurde nicht gefunden!");
        return;
    }
    Serial.println(" - Datei (aprm2.htm) wurde gefunden.");

    Serial.println();
    Serial.println("Verbraucher schalten");
}

void loop()
{
    EthernetClient client = server.available();
    	// Auf Anfrage warten

    if(client)
    {
        /*****************************************
          Ausgaenge ueber das Webformular steuern  *
        *****************************************/
        TextFinder finder(client);

        if(finder.find("GET"))
        {
            while(finder.findUntil("cmd-", "\n\r"))
            {
              //String prot = client.read();
             // int  bits = finder.getValue();
            //  int  value = finder.getValue();  
           //   send_ir_data(prot, bits, value);
             Serial.println("CMD-");
            }
        }

        /************************
          Webformular anzeigen  *
        ************************/
    
        boolean current_line_is_blank = true;
			// eine HTTP-Anfrage endet mit einer Leerzeile und einer neuen Zeile
        while (client.connected())
        {
            if (client.available())
            	// Wenn Daten vom Server empfangen werden
            {
                read_line(client, line, MAX_LINE);
                char c = client.read();
                	// empfangene Zeichen einlesen
                if (c == '\n' && current_line_is_blank)
                	// wenn neue Zeile und Leerzeile empfangen
                {
                    // Standard HTTP Header senden
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type: text/html");
                    client.println("Connection: close");
                    client.println();
                    // Website von SD-Karte laden
                    webFile = SD.open("aprm2.htm");
                    	// Website laden
                    if (webFile)
                    {
                        while(webFile.available())
                        {
                            client.write(webFile.read());
                            	// Website an Client schicken
                        }
                        webFile.close();
                    }
                    break;
                }
                if (c == '\n')
                {
                    current_line_is_blank = true;
                }
                else if (c != '\r')
                {
                    current_line_is_blank = false;
                }
            }
        }
        delay(1);
        client.stop();
    }
}

IRsend _infrared_sender;
    void read_line(EthernetClient& client, char* buffer, const int buffer_length)
    {
        int buffer_pos = 0;
        while (client.available() && (buffer_pos < buffer_length - 1))
        {
            const char c = client.read();
            if (c=='\n')
                break;
            if (c!='\r')
                buffer[buffer_pos++] = c;
        }
        buffer[buffer_pos] = '\0';                                                        
    }

