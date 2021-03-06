#include <SPI.h>
#include <Ethernet.h>
#include <IRremote.h>
#include <TextFinder.h>
#include <SD.h>

// ### Voraussetzungen ###
// TSOP Signal-Pin <--> Arduino - Pin 11
// IR-LED Anode <--> Arduino - Pin 3
// Test-LED <--> Arduino - Pin 6

const int chipSelect = 4;
const unsigned int PROXY_PORT = 80;
const unsigned int BAUD_RATE = 19200;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE }; // MAC Arduino Ethernet (David)
byte ip[] = { 192, 168, 3, 100 };
EthernetServer server(PROXY_PORT);
File webFile;

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

void send_html(EthernetClient& client)
{
    client.println("HTTP/1.1 200 OK\n");
}

void zeige_html(EthernetClient& client)
{
    Serial.println("HTML-ZEIGEN!!!");
    boolean current_line_is_blank = true;       // eine HTTP-Anfrage endet mit einer Leerzeile und einer neuen Zeile

    while (client.connected())
    {
        Serial.println("CLIENT CONNECTED");
        if (client.available())                   // Wenn Daten vom Server empfangen werden
        {
            Serial.println("CLIENT AVAILABLE");
            char c = client.read();                 // empfangene Zeichen einlesen
            if (c == '\n' && current_line_is_blank) // wenn neue Zeile und Leerzeile empfangen
            {
                // Standard HTTP Header senden
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();

                // Website von SD-Karte laden
                webFile = SD.open("sd.htm");  // Website laden
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

bool send_ir_data(const char* protocol, const int bits, const long value)
{
    bool result = true;
    if (!strcasecmp(protocol, "NEC"))
        _infrared_sender.sendNEC(value, bits);
    else if (!strcasecmp(protocol, "SONY"))
        _infrared_sender.sendSony(value, bits);
    else if (!strcasecmp(protocol, "RC5"))
        _infrared_sender.sendRC5(value, bits);
    else if (!strcasecmp(protocol, "RC6"))
        _infrared_sender.sendRC6(value, bits);
    else
        result = false;
    return result;
}

bool handle_command(char* line)
{
    strsep(&line, "/");
    char* path = strsep(&line, "/");
    char* args[3];
    for (char** ap = args; (*ap = strsep(&path, "?")) != NULL;)
        if (**ap != '\0')
            if (++ap >= &args[3])
                break;
    const int bits = atoi(args[1]);
    const long value = atol(args[2]);
    return send_ir_data(args[0], bits, value);
}

void receive_from_server(EthernetClient client)
{
    const int MAX_LINE = 256;
    char line[MAX_LINE];
    if (client)
    {
        while (client.connected())
        {
            if (client.available())
            {
                read_line(client, line, MAX_LINE);
                Serial.println(line);
                if (line[0] == 'G' && line[1] == 'E' && line[2] == 'T')
                    handle_command(line);
                if (!strcmp(line, ""))
                {
                    send_html(client);
                    break;
                }
            }
        }
        delay(1);
    }
}


void setup()
{
// Open serial communications and wait for port to open:
    Serial.begin(BAUD_RATE);
    // start the Ethernet connection and the server:
    Ethernet.begin(mac);
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());

    Serial.print("Initializing SD card...");
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(10, OUTPUT);

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        return;
    }
    Serial.println("card initialized.");

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
}

void loop()
{
    EthernetClient client = server.available();
    if(client)
    {
        receive_from_server(client);
        zeige_html(client);
        client.stop();
    }
}

