# AltiDuoMini
Firmware for the AltiDuo Mini altimeter using an ATtiny 84  microcontroller
This is another version of the Alti Duo that is using an Attiny 84 processor. It is smaller than the other AltiDuo that is using an ATMega 328. However unlike the big AltiDuo it cannot connect to the USB port so to program it you have to remove the ship.

The original version uses a BMP085 pressure sensor.           
<img src="/pictures/Alti duo-bmp085.jpg" width="49%">

The latest revision uses a BMP180 pressure sensor, however the code for both board is identical.
<img src="/pictures/altiduo-bmp180.jpg" width="49%">

# Building the code
You will need to download the Arduino ide from the [Arduino web site](https://www.arduino.cc/).
You need to download the [Attiny support](https://code.google.com/archive/p/arduino-tiny/downloads) and install it to your Arduino environement.
You have to load the Arduino Attiny84 boot loader to your ATtiny84 micro controller. 
Make sure that you download the following [support libraries](https://github.com/bdureau/AltimetersLibs) tinyBMP085, tinyWireM, tinyWireS and copy them to the Arduino library folder. To compile it you need to choose the Attiny 84 and the correct USB port.
You will need to use an AVR programmer and an adapter to program the microcotroller, refer to the documentation.
