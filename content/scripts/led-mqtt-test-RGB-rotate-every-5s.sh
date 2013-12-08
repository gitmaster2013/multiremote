#!/bin/sh

mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16769055"

while true
do
  mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16748655"
  sleep 5
  mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16716015"
  sleep 5
  mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16732335"
  sleep 5
done

mosquitto_pub -h test.mosquitto.org -t "MDinTopic" -m "CE/?0001?0001?NEC?32?16736415"
