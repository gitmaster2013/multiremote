// http://fluuux.de/2013/03/arduino-als-webserver-einrichten-und-webpage-von-sd-karte-laden/
#if ARDUINO > 18
#include <SPI.h> // Für Arduino Version größer als 0018
#endif
#include <Ethernet.h>
#include <TextFinder.h>
#include <SD.h>
#include <IRremote.h>

// ### Voraussetzungen ###
// TSOP Signal-Pin <--> Arduino - Pin 11
// IR-LED Anode <--> Arduino - Pin 3
// Test-LED <--> Arduino - Pin 6

const unsigned int PROXY_PORT = 80;
const unsigned int BAUD_RATE = 19200;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };
// MAC Arduino Ethernet (David)
// pinMode(10, OUTPUT);
// make sure that the default chip select pin is set to
// output, even if you don't use it:
byte sdPin = 4;
// Pin der SD-Karte

EthernetServer server(PROXY_PORT);
// Server port

File webFile;

bool send_ir_data(const char* protocol, const int bits, const long value)
{
    Serial.print("Befehl empfangen");
    Serial.print(" <-- Protocol: ");
    Serial.print(protocol);
    Serial.print(", Bits: ");
    Serial.print(bits);
    Serial.print(", Value: ");
    Serial.println(value);
    Serial.println();
    IRsend ir_sender;
    ir_sender.sendNEC(value, bits);
}

bool handle_command(char* line)
{
    strsep(&line, "/");
    // Der erste "/" nach der IP wird entfernt
    char* path = strsep(&line, "/");
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
        // DEBUG: Alle IR-Sende-Parameter werden auf der Konsole ausgegeben
        return send_ir_data(args[2], bits, value);
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

void read_html(EthernetClient& client)
{
    Serial.println("IN READ HTML");
    const int MAX_LINE = 256;
    char line[MAX_LINE];

    read_line(client, line, MAX_LINE);
    Serial.println(line);

    if (line[0] == 'G' && line[1] == 'E' && line[2] == 'T')
    {
        handle_command(line);
    }
}

void write_html(EthernetClient& client)
{
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
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();

                webFile = SD.open("aprmm.htm");
                // Website von SD-Karte laden
                if (webFile)
                {
                    while(webFile.available())
                    {
                        client.write(webFile.read());
                        // Website an Client schicken
                    }
                    webFile.close();
                    Serial.println("Seite fertig!");
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
}


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

    if (!SD.exists("aprmm.htm"))
    {
        Serial.println(" - Datei (aprmm.htm) wurde nicht gefunden!");
        return;
    }
    Serial.println(" - Datei (aprmm.htm) wurde gefunden.");

    Serial.println();
    Serial.println("Verbraucher schalten");
}

void loop()
{
    Serial.println("LOOP");
     EthernetClient client = server.available();
    // Auf Anfrage warten

    if(client)
    {
      Serial.println("CLIENT erkannt");
        /*****************************************
          Ausgänge über das Webformular steuern  *
        *****************************************/
        read_html(client);
        Serial.println("NACH HTML");

        /************************
          Webformular anzeigen  *
        ************************/
        write_html(client);

        delay(1);
        client.stop();
    }
}

