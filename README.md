# Midi Floppy Music Controller
This is a midi compatible Floppymusic controller for the arduino due.
Inspired by github.com/SammyIAm/Moppy but with real standalone midi features.
Just flash it to you arduino due, edit it according to your needs (maybe remove lcd portions), connect the direct usb port and play from any midi program/editor like midi trail or sekaiju.

The drives are separate midi channels that should play only one note at a time.
Pitch bends are supported and there is basic handling of overlapping notes.

## How to connect floppy drives:
The big end pin row with pins 22 to 53 is used here.
Connect the even pins to the step pins on the drive (pin 20 on drive) and the odd pins to the direction pins (pin 18 on drive).

Place a jumper between pin 11 and 12 below to enable the drive.

The drive only needs 5v so leave the 12v pin disconnected. You can power it from an atx power supply or a strong 5v source. An arduino from usb is NOT powerful enough and might get damaged. At least for more than one drive.
So make sure to never connect the drives 5v power supply to the arduino if only the arduino is powered. I advise to use a diode for protection or not connect the power at all. The ground rails must of course be connected between the floppy power supply and the arduino.
