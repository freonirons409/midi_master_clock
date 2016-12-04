# Arduino Midi Master Clock
=============
Arduino project for create a master midi clock generator with start/stop and tap tempo functionality.

##Parts Needed
-------
* Arduino Mega 2560 R3 (https://www.arduino.cc/en/Main/ArduinoBoardMega2560)
* Sparkfun Midi Shield kit (https://www.sparkfun.com/products/12898)
* Arduino LCD 1602A 16x2 (https://www.adafruit.com/products/181)
* 10k Ohm 0.5W, 1/2W PC Pins Through Hole Trimmer Potentiometer Cermet 1 Turn Top Adjustment (included with most LCD kits);
* Strip of header
* PCB Board
* Jumper wire

##Assembly
-------
1. Follow the necessary steps to assemble the LCD screen, and the Sparkfun MIDI shield per the instructions included with each one. On the MIDI shield, you can leave off the POT for A0 if you want, it will not be used for this project. Also recommend not mounting the MIDI I/O connectors directly to the board but instead use wire so that they can be moved around inside whatever encloser you wanted.  Same with any of the pots and buttons on the MIDI shield--leave them free from the board if you have an enclosure in mind. They are a bitch to take off later.
2. On the LCD header strip, invert the pin so that it does not go into Arduino input 51, but instead sticks out the opposite direction--this will have a jumper cable attached to it later.
0. Insert the LCD strip header pins into the bottom row of Digital I/O on the Arduino Mega board so that PIN 1 of the LCD goes into the first GND pin, then the rest should line of as follows:

```
1	|	2	|	3	|	4	|	5	|	6	|	7	|	8	|	9	|	10	|	11	|	12	|	13	|	14	|	15	|	16	|

GND	|	53	|	-	|	49	|	47	|	45	|	43	|	41	|	39	|	37	|	35	|	33	|	31	|	29	|	27	|	25	|
```

3. Place the midi shield in the proper input slots matching what is on the Shield board.
4. Cut a piece of PCB board so that it is the width of the arduino. It should fit in the space between the MIDI sheild and the LCD screen and to where the edges line up with the Digital and Analog inputs in this blank section. On the extra piece of PCB board, solder the 10k Potentiometer and a strip of header with 3 pins is connected to Analog inputs A8-A10, but you will not be using A9.
5. solder a connection to where jumper pin for A8 goes to the first pin of the POT, and then jumper for A10 is going to the third pin on the POT. These analog outputs will be supplying the GND and POWER to the POT.
6. Using the jumper wire, connect the middle pin of the POT to the pin 3 of the LCD--the one you inverted and have sticking outward from step 2. This connection allows you to control the LCD contrast with the POT.
7. Use the leftover strip header to secure the PCB board to the arduino using the unused digital inputs 14-21. This is just to hold the board in place, they will not be used later.
8. Upload the code in the midi_master_clock.ino code block to the arduino--make sure the switch on the MIDI shield is switched to PROG and not RUN so that the memory can be accessed.
9. The code will automatically set the INPUTS and OUTPUTS where the LCD connects for data transfer and power.  The LCD should power up with a message and the current BPM.
10. Use the Middle Button (D3) to send a START/STOP message to your synth/sampler
11. Button D2 is not used, but could be used to send other MIDI signals if you wanted.
12. Use the Third button (D4) to tap tempo
13. Use the POT A1 to manually dial in the tempo.


That's it!  Email me at hey@aaronirons.net if you have any questions.
https://www.aaronirons.net
https://instagram.com/freonirons409


