#if ARDUINO > 18
#include <SPI.h> // Für Arduino Version größer als 0018
#endif
#include <Ethernet.h>
#include <TextFinder.h>
#include <SD.h>
#include <IRremote.h>
 
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xDB, 0xAE };
// MAC Arduino Ethernet (David)
byte sdPin = 4;
// Pin der SD-Karte
const unsigned int PROXY_PORT = 80;
const unsigned int BAUD_RATE = 19200;
 
EthernetServer server(PROXY_PORT);
 
File webFile;
 
void setup()
{  
  Ethernet.begin(mac);
  // Client starten
  server.begin();
  // Server starten
  Serial.begin(BAUD_RATE);
  Serial.println("ARDUINO - STEUERUNG");
 
  Serial.println("Initialisiere SD-Karte...");
  if (!SD.begin(sdPin)) 
  {
    Serial.println(" - Initialisierung der SD-Karte fehlgeschlagen!");
    return;
  }
  Serial.println(" - SD-Karte erfolgreich initialisiert.");
 
  if (!SD.exists("aprmir.htm")) 
  {
    Serial.println(" - Datei (aprmir.htm) wurde nicht gefunden!");
    return;
  }
  Serial.println(" - Datei (aprmir.htm) wurde gefunden.");
 
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
 
        if(val==1) Serial.println(" ein"); else Serial.println(" aus");
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
        { // Standard HTTP Header senden
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
 
          // Website von SD-Karte laden
          webFile = SD.open("aprmir.htm");  // Website laden
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
