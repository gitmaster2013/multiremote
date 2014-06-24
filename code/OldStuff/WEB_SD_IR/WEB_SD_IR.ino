// http://fluuux.de/2013/03/arduino-als-webserver-einrichten-und-webpage-von-sd-karte-laden/

#include <Ethernet.h>
#include <TextFinder.h>
#include <SD.h>
#include <IRremote.h>

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

    void send_command(const long command)
    {
        mac.sendNEC(command, CMD_LEN);
    }

public:
    void menu()
    {
        send_command(MENU);
    }
    void play()
    {
        send_command(PLAY);
    }
    void prev()
    {
        send_command(PREV);
    }
    void next()
    {
        send_command(NEXT);
    }
    void up()
    {
        send_command(UP);
    }
    void down()
    {
        send_command(DOWN);
    }
};

AppleRemote apple_remote;

const unsigned int PROXY_PORT = 80;
const unsigned int BAUD_RATE = 19200;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };
	// MAC Arduino Ethernet (David)
byte sdPin = 4;
	// Pin der SD-Karte

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

    if (!SD.exists("aprm.htm"))
    {
        Serial.println(" - Datei (aprm.htm) wurde nicht gefunden!");
        return;
    }
    Serial.println(" - Datei (aprm.htm) wurde gefunden.");

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
                char befehl = client.read();
                Serial.print(" - D"+String(befehl));
                switch(befehl)
                {
                case 'm':
                    apple_remote.menu();
                    break;
                case 'u':
                    apple_remote.up();
                    break;
                case 'd':
                    apple_remote.down();
                    break;
                case 'l':
                    apple_remote.prev();
                    break;
                case 'r':
                    apple_remote.next();
                    break;
                case 'p':
                    apple_remote.play();
                    break;
                default:
                    Serial.print(" - Falscher Befehl");
                    break;
                }
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
                    webFile = SD.open("aprm.htm");
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

