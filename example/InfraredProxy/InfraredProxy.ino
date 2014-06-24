#include <SPI.h>
#include <Ethernet.h>
#include <IRremote.h>
// ### Voraussetzungen ###
// TSOP Signal-Pin <--> Arduino - Pin 11
// IR-LED Anode <--> Arduino - Pin 3
class InfraredProxy
{
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
        strsep(&line, " ");
        char* path = strsep(&line, " ");
        char* args[3];
        for (char** ap = args; (*ap = strsep(&path, "/")) != NULL;)
            if (**ap != '\0')
                if (++ap >= &args[3])
                    break;
        const int bits = atoi(args[1]);
        const long value = atol(args[2]);
        return send_ir_data(args[0], bits, value);
    }
public:
    void receive_from_server(EthernetServer server)
    {
        const int MAX_LINE = 256;
        char line[MAX_LINE];
        EthernetClient client = server.available();
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
                        client.println("HTTP/1.1 200 OK\n");
                        break;
                    }
                }
            }
            delay(1);
            client.stop();
        }
    }
};
//--- ENDE DER DEKLARATION ---
const unsigned int PROXY_PORT = 80;
const unsigned int BAUD_RATE = 19200;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE }; // MAC Arduino Ethernet (David)
byte ip[] = { 192, 168, 3, 100 };
EthernetServer server(PROXY_PORT);
InfraredProxy ir_proxy;
void setup()
{
// Open serial communications and wait for port to open:
    Serial.begin(BAUD_RATE);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for Leonardo only
    }
    // start the Ethernet connection and the server:
    Ethernet.begin(mac);
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
}
void loop()
{
    ir_proxy.receive_from_server(server);
}
