/*--------------------------------------------------------------
  Program:      eth_websrv_SD_link

  Description:  Arduino web server that serves up a basic web
                page that links to a second page. Clicking the
                link will open the second page. The second page
                links back to the first page.
  
  Hardware:     Arduino Uno and official Arduino Ethernet
                shield. Should work with other Arduinos and
                compatible Ethernet shields.
                2Gb micro SD card formatted FAT16
                
  Software:     Developed using Arduino 1.0.5 software
                Should be compatible with Arduino 1.0 +
                
                Requires page1.htm and page2.htm to be on the
                micro SD card in the Ethernet shield micro
                SD card socket.
  
  References:   - WebServer example by David A. Mellis and 
                  modified by Tom Igoe
                - SD card examples by David A. Mellis and
                  Tom Igoe
                - Ethernet library documentation:
                  http://arduino.cc/en/Reference/Ethernet
                - SD Card library documentation:
                  http://arduino.cc/en/Reference/SD

  Date:         2 March 2013
  Modified:     14 June 2013
                - removed use of String class, used too much SRAM
                - added StrClear() and StrContains() functions
                - disable Ethernet chip at startup
 
  Author:       W.A. Smith, http://startingelectronics.com
--------------------------------------------------------------*/

  #include <SPI.h>
  #include <Ethernet.h>
  #include <SD.h>
  //#include <IRremote.h>
  //#include <IRLib.h>
  //IRsend irsend;
  
  // size of buffer used to capture HTTP requests
  #define REQ_BUF_SZ   64
  
  const unsigned int PROXY_PORT = 80;
  const unsigned int BAUD_RATE = 19200;
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };
  // MAC Arduino Ethernet (David)
  EthernetServer server(PROXY_PORT);       // create a server at port 80
  File webFile;                    // handle to files on SD card
  char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
  char req_index = 0;              // index into HTTP_req buffer
  
  void setup()
  {
      // disable Ethernet chip
      pinMode(10, OUTPUT);
      digitalWrite(10, HIGH);
      
      Serial.begin(BAUD_RATE);       // for debugging
      
      // initialize SD card
      Serial.println("Initializing SD card...");
      if (!SD.begin(4)) {
          Serial.println("ERROR - SD card initialization failed!");
          return;    // init failed
      }
      Serial.println("SUCCESS - SD card initialized.");
      // check for page1.htm file
      if (!SD.exists("page1.htm")) {
          Serial.println("ERROR - Can't find page1.htm file!");
          return;  // can't find index file
      }
      Serial.println("SUCCESS - Found page1.htm file.");
  
      Ethernet.begin(mac);  // initialize Ethernet device
      server.begin();           // start to listen for clients
      // start the Ethernet connection and the server:
      Serial.print("Server is at: ");
      Serial.println(Ethernet.localIP());
      // Server starten
      Serial.println("ARDUINO - WEB SD MULTI");
  }

void loop()
{
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                Serial.print(c);    // print HTTP request character to serial monitor
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connnection: close");
                    client.println();             
                    // open requested web page file
                    int page = handle_command(HTTP_req);
                    if (page == 1)
                    {
                        webFile = SD.open("page1.htm");        // open web page file
                    }
                    else if (page == 2)
                    {
                        webFile = SD.open("page2.htm");        // open web page file
                    }
                    else
                    {
                        webFile = SD.open("page3.htm");        // open web page file
                    }
                    // send web page to client
                    if (webFile) {
                        while(webFile.available()) {
                            client.write(webFile.read());
                        }
                        webFile.close();
                    }
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

int handle_command(char* line)
{
    strsep(&line, "/"); //Pointer von "/"
    char* path = strsep(&line, "/");  // Beginn von path auf das erste "/"
    // Das erste Zeichen des zu parsenden Strings ist das nach dem entfernten "/" 
    Serial.println(path);
    
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
        Serial.println();
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
    return device;
}
