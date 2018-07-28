# MaplinSockets

Control of Maplin 433Mhz remote control sockets with an ESP8266-01

# API

http://ESP8266IPAddress/?group=[a-d]&socket=[1-4]&state=[on/off]

# 433Mhz transmitter

Uses a simple on off key (OOK) 433Mhz transmitter.

Connect:

  GND to GND  
  VCC to GPIO 1  (RX Data)
  Data to GPIO 3 (TX Data)
