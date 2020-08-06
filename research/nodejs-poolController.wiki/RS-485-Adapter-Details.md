This article goes into more detail on the RS-485 adapter that you need to run this code.  

## Step 1

Read the README carefully!  It mentions the need for a physical RS485 adapter as the [first line in the install instructions](https://github.com/tagyoureit/nodejs-poolController#installation-instructions) and goes into further detail [here](https://github.com/tagyoureit/nodejs-poolController#rs485-adapter).

## Step 2

Do you own an RS485 adapter?  This is different from a standard serial port adapter.  I recommend this [JBTek](https://www.amazon.com/JBtek-Converter-Adapter-ch340T-Supported/dp/B00NKAJGZM/ref=sr_1_1?ie=UTF8&qid=1524495023&sr=8-1&keywords=jbtek+rs485) for $6.99 on Amazon.

## Step 3

You have your RS-485 adapter now, great!  Can you see when you browse devices (on Windows) or does it show up in the `/dev` directory on Mac/Linux? 
 
### Mac

Before you plug in the adapter, it will not be in `/dev`.

[[images/dev_dir_no_adapter.png]]

Then plug it in and you will see the adapter listed.  If it will show up as `ttyUSB0` for the JBTek.  Other adapters may have different names. 

[[images/dev_dir_with_adapter.png]]

If it doesn't show up when you insert it, you probably need to figure out what chip your USB adapter uses.
Two of the bigger ones are:

* CH340x: https://sparks.gogo.co.nz/ch340.html (This is my recommended JBTek chipset.)
* FTDI: http://www.ftdichip.com/Drivers/VCP.htm (First install the [CDM v2.12.28 WHQL Certified driver](http://www.ftdichip.com/Drivers/CDM/CDM%20v2.12.28%20WHQL%20Certified.zip) to have Windows recognize the USB serial device, then install http://www.ftdichip.com/Drivers/CDM/CDM21228_Setup.zip to recognize the serial port itself.)
Of course, install the drivers, reboot, yadda yadda...

## Windows/FTDI

Here are the screenshots of installing the FTDI driver on my Win7 image.

* Plug in the FTDI USB Serial Port, and it isn't recognized by Windows.

[[images/win_ftdi_1.png]]

* Install CDM v2.12.28 WHQL Certified driver and Windows and the USB device is now recognized, but not the Serial Port itself.

[[images/win_ftdi_usb.png]]

* Install the [drivers](http://www.ftdichip.com/Drivers/CDM/CDM21228_Setup.zip) and then the serial port will be recognized.  (Note: I did these two steps separately but this last setup package may install both USB and Serial Port drivers.)

[[images/win_ftdi_recognized.png]]

## Step 4

Connect the wires.  Once you see the driver listed in the /dev directory, you should connect the RS485 wires physically to your pool. Connect the DATA+ and DATA- either to your remote keypad, the Intellitouch itself, or to the protocol adapter of the Screenlogic Wireless.

There are only two possible ways to wire the DATA+ and DATA- wires.  To see if you are getting the proper communications from the bus, before you even try to run this program, run from your unix command line

### Mac

`od -x < /dev/ttyUSB0`  Of course, you'll need to change the address of your RS-485 adapter if it isn't the same as mine (here and in the code).  For example, `od -x < /dev/tty.usbserial-AIZ9PCX` is the name of the RS485 adapter that showed up in my screenshot above.

You'll know you have the wires right when the output of this command looks like (you should see multiple repetitions of ffa5ff):
```
0002240 0000 0000 0000 0000 0000 ff00 ffff ffff
0002260 **ffff 00ff a5ff** 0f0a 0210 161d 000c 0040
0002300 0000 0000 0300 4000 5004 2050 3c00 0039
0002320 0400 0000 597a 0d00 af03 00ff a5ff 100a
0002340 e722 0001 c901 ffff ffff ffff ffff ff00
```

This is the WRONG wiring (no ffa5ff present).
```
0001440 0000 0000 0000 0000 0000 0000 0000 6a01
0001460 e1d6 fbdf d3c5 fff3 ff7f ffff ffff f9ff
0001500 7fff 5ff7 bf5f 87ff ff8d f7ff ffff 4d0b
0001520 e5ff adf9 0000 0000 0000 0000 0100 d66a
0001540 dfe1 c5fb f3d3 7fff ffff ffff ffff fff9
```

### Windows

I recommending downloading [RealTerm](http://realterm.sourceforge.net/) as it worked easily for me the first time.  When you open it, make sure to have HEX selected as the output.

[[images/realterm1.png]]

And the port settings should be 9600 baud, no parity, 8 data bits, 1 stop bit, no flow control. If you have the wires connected correctly, you will see (red circle) the FF00FFA5 repeated a number of times.  If you don't see these, just reverse the wires on one end.

[[images/realterm2.png]]

## Step 5

Now that you have your RS485 adapter configured and verified in the operating system, change the line in your config.json

...
"network": {
            "rs485Port": "/dev/ttyUSB0",
to match the port you see above.

## Step 6

Run the nodejs-poolController app and it should talk to your Pentair pool system.