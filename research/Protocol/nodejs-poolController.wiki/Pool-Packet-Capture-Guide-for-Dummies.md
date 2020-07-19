# Want to help decode certain packets?  We need your help!  Why?  
* It's not easy
* You may have equipment (or a different configuration) than others
* You're smarter than us
* Controlling a certain aspect of equipment XYZ is the last mile in your journey to completely automate your life and you will just die if you don't get it done
* It's the right thing to do

# Where do you start?
When you have your poolController app running, navigate to `http://{ip_of_poolcontroller}:3000/public/debug.html`.  Right now, it's just a boring, non-formatted screen.

[[images/Debug-StartingScreen.png]]

Input the:
* Destination & Source - You can find the (addresses here)[Address].
* Action - You can find the (Actions here)[Broadcast].

For example, entering:
* Destination 15 (Broadcast)
* Source 16 (Controller)
* Action 2 (Status)
results in the following

[[images/Debug-StatusPacket.png]]

Of course, press `Start Listening` or `Stop Listening` to start/stop the stream.

As a quick reminder, here is the format of an example packet:  165,33,15,16,2,29,...3,197

|Position in packet| Example Byte | Definition |
|--|--|--|
|0|165 | Start of packet|
|1|33 | Bit that identifies the system (can change)|
|2|15 | Destination|
|3|16 | Source|
|4|2 | Action|
|5|29 | Length|
|6 through 3rd from last|... | All of the data/information packets|
|2nd to last,last|3, 197 | Checksum (always the last two bytes)|

# I have some packets, now what...???
Many (but not all) are documented on the [[Broadcast]] page.  If there is something that isn't working right, or not available yet in this app, then you can use this tool to help figure it out.  How, you ask?
1. Capture the packets before you take any action.
1. Press a button, turn something on, change something, etc.
1. See what packets change
1. Post your findings as a [new issue](https://github.com/tagyoureit/nodejs-poolController/issues/new) or chat about it with us on [Gitter.im](https://gitter.im/nodejs-poolController/Lobby).

## Limitations
* This is only for controller packets.  It doesn't work with chlorinator packets.

## What would make this more useful?
* Listen to get/set/status packets all at the same time?
* Listen to multiple packets (different actions)?
* UI is ugly.  I want you to make it better!
* Give it to me in a format I can put in Excel
* Have it tell me when a byte changes compared to the last packet.
* Number the packets (eg 1: x, 2: xy, 3: xyz) or put it in a table format