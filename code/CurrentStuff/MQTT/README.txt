README:
Aktuell wird ein lokaler Server verwendet. Es kann jedoch auch der mosquitto Test Server verwendet werden.


Dazu muss die IP im Sketch angepasst werden!
byte server[] = { 85, 119, 83, 194 };

#define CLIENTID "MDArduino"
#define INTOPIC "MDinTopic"
#define OUTTOPIC "MDoutTopic"

So setzt man Steuercodes ab:
CODE-RGB-AUS: mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16736415"
CODE-RGB-ON: mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16716015"