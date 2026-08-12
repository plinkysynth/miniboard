# miniboard - a super minimal fatar tp9 37 key keybed scanner using an rp2040 mcu by @mmalex.bsky.social

august 2026

![miniboard](pic.webp)

## keybed

the keybed - https://www.fatar.com/products/tp9s/
datasheet - https://www.antonus-synths.com/enlo-1/fatar/DataSheet-TP9S%2BP.pdf

## project

forked from my earlier zacboard project, that added an oled, 2 encoders, and i2c slider boards.
im not sure if i ever released that one, but i was curious about making a super minimal 'fatar keybed and nothing else'

## software

the software just times make/breaks when scanning the keybed, and uses that to generate velocity
it also feeds the aftertouch resistor to ADC0 and uses that to generate aftertouch events.
it sends MIDI messages out on channel 0 on the TRS port and over usb midi.

the pcb has a 3 pin connector for a raspberry pi debug probe 
https://www.raspberrypi.com/products/debug-probe/
which I highly recommend for all rp2040 hacking projects. it gives you clean debugging in vscode, for example.
i set this project to use rtt segger style printf, which menas that if you have a probe connected you can
see your printf's in vscode via the TERMINAL tab, and its really cheap, and doesnt tie up the USB port or a 
serial port. lovely!

## hardware

power is over ucb-c obviously

i did this in a yolo rush, so its a bit rough.
ordering your own pcbs, its the usual kicad -> jlcpcb -> yay!
all the parts should be readily available via their assembly service.

## mounting

the open scad file is for a simple plank to screw onto the keybed and mount the circuit board.
a plank of wood or other material would be nicer.
i screwed it into the keybed with 8 wood screws :) M3 or M4 I think they were.
the pcb has two m3 holes in it, which I used 2 M3 machine screws and nuts for.

## keybed connector

i HATE HATE HATE the minimatch connector that fatar uses, but here we are. you need some 20 way
ribbon cable to clamp it onto, which requires FORCE which often breaks the fragile plastic.
I got the connectors from digikey:
for the cable...
2x A99456CT-ND  TE CONNECTIVITY AMP (VA) / 9-215083-0 CONN PLUG 20POS IDC 28AWG TIN

for the pcb...
1x A99465CT-ND  TE CONNECTIVITY AMP (VA) / 9-215079-0 CONN RCPT 20POS 0.1 TIN PCB

and standard 28AWG 20 way ribbon cable.

The keybed also has a little flat flex variable resistor. connect two wires to it
and solder them to the outer two pins of the 4 pin connector on the pcb. voila,
after touch.

## PCB design mistakes chapter:

i should have connected the last 2 pins to GPIOs. I didn't. as a result you can't scan larger keyboards.
If i was ordering from JLCPCB again, I'd hook up those two not connected pins to GPIOs.

obviously it could sprout more connections, encoders, buttons, etc.... but that wasnt the point of 
this minimalist project.  still, it would make sense to expose more of the free GPIOs as testpads 
or connectors.

have fun!

## license

released into as liberal a license as I can: [CC0](LICENSE.md)...

- alex 'mmalex' evans august 2026 
