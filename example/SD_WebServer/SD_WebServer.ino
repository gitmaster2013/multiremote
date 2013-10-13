// http://fluuux.de/2013/03/arduino-als-webserver-einrichten-und-webpage-von-sd-karte-laden/

#if ARDUINO > 18
#include <SPI.h> // Für Arduino Version größer als 0018
#endif
#include <Ethernet.h>
#include <TextFinder.h>
#include <SD.h>

byte mac[] = { 0x5A, 0xA2, 0xDA, 0x0D, 0x56, 0x7A }; // MAC-Adresse des Ethernet-Shield
byte ip[]  = { 192, 168, 2, 102 };                   // IP zum aufrufen des Webservers
byte sdPin = 4;                                      // Pin der SD-Karte

EthernetServer server(80);                           // Server port

File webFile;

void setup()
{
    Ethernet.begin(mac, ip); // Client starten
    server.begin();          // Server starten
    Serial.begin(9600);
    Serial.println("ARDUINO - STEUERUNG");

    Serial.println("Initialisiere SD-Karte...");
    if (!SD.begin(sdPin))
    {
        Serial.println(" - Initialisierung der SD-Karte fehlgeschlagen!");
        return;
    }
    Serial.println(" - SD-Karte erfolgreich initialisiert.");

    if (!SD.exists("index.htm"))
    {
        Serial.println(" - Datei (index.htm) wurde nicht gefunden!");
        return;
    }
    Serial.println(" - Datei (index.htm) wurde gefunden.");

    Serial.println();
    Serial.println("Verbraucher schalten");
}

void loop()
{
    EthernetClient client = server.available(); // Auf Anfrage warten

    if(client)
    {
        /*****************************************
          Ausgänge über das Webformular steuern  *
        *****************************************/
        TextFinder finder(client);

        if(finder.find("GET"))
        {
            while(finder.findUntil("pin", "\n\r"))
            {
                char typ = client.read();
                int  pin = finder.getValue();
                int  val = finder.getValue();

                if(typ == 'D')
                {
                    pinMode(pin, OUTPUT);
                    digitalWrite(pin, val);
                    Serial.print(" - D"+String(pin));
                }
                else if(typ == 'A')
                {
                    analogWrite(pin, val);
                    Serial.print(" - A"+String(pin));
                }
                else Serial.print(" - Falscher Typ");

                if(val==1) Serial.println(" ein");
                else Serial.println(" aus");
            }
        }

        /************************
          Webformular anzeigen  *
        ************************/
        boolean current_line_is_blank = true;       // eine HTTP-Anfrage endet mit einer Leerzeile und einer neuen Zeile

        while (client.connected())
        {
            if (client.available())                   // Wenn Daten vom Server empfangen werden
            {
                char c = client.read();                 // empfangene Zeichen einlesen
                if (c == '\n' && current_line_is_blank) // wenn neue Zeile und Leerzeile empfangen
                {
                    // Standard HTTP Header senden
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");
                    client.println();

                    // Website von SD-Karte laden
                    webFile = SD.open("index.htm");  // Website laden
                    if (webFile)
                    {
                        while(webFile.available())
                        {
                            client.write(webFile.read()); // Website an Client schicken
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
