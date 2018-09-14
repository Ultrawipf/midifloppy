# Midi Floppy Music Controller
Floppy Music controller für arduino (due) based on moppy and midiusb.
Just flash it to you arduino due, edit it according to your needs, connect the direct usb port and play from any midi program/editor like midi trail or sekaiju.

The drives are separate midi channels that should play only one note at a time.

## How to connect floppy drives:
Connect the even pins to the step pins on the drive (pin 20 on drive) and the odd pins to the direction pins (pin 18 on drive).

Place a jumper between pin 12 and the pin below to enable the drive.

The drive only needs 5v so leave the 12v pin disconnected. You can power it from an atx power supply or a strong 5v source. An arduino from usb is NOT powerful enough and might get damaged. At least for more than one drive.
