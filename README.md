Barometer Project
http://www.smbaker.com/

This project implements a barometer over 433Mhz wireless.
The idea is to make many sensors (30 or so) and put them in my 3D filament containers.
Each sensor uses a BME280 as the sensor, a ATTINY85 as the CPU, and a FS1000A as the transmitter.

On a raspberry pi, we use a 433Mhz receiver to receive and decode the packets.

baromdisp-85
My current working version uses the ATTINY85.
You'll need to use my fork of TinyBME280 as I added support for sleep mode.


baromdisp-328
My original prototype using an ATMEGA328.
Also supported an ePaper display.
Too big and consumed too much power.
