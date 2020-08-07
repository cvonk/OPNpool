# Messages originated from the Pump(s)

This page lists the messages that are sent by the pump.  This application is built around systems that have a modern Pentair pool controller (EasyTouch, et al).  ~~If you want to control the pumps directly, without the use of a pool controller, see this [CocoonTech thread](http://cocoontech.com/forums/topic/13548-intelliflow-pump-rs485-protocol) (registration required).~~ This application will now read all of the pump commands.  

Action | Mode\Command | Description
---|---|---
<01>|  <02><96>     | Speed mode set to <xx><xx>
<01>|  <03><33> | Run the following Ext. Program <00><00>=Off, <00><08>=1, <00><10>=2, <00><18>=3, <00><20>=4
<01>|  <03><39> | Set Speed Setting Program 1 to speed <xx><xx> in rpm
<01>|  <03><40> | Set Speed Setting Program 2 to speed <xx><xx> in rpm
<01>|  <03><41> | Set Speed Setting Program 3 to speed <xx><xx> in rpm
<01>|  <03><42> | Set Speed Setting Program 4 to speed <xx><xx> in rpm
<01>|  <03><43> | set pump timer to <xx><xx> minutes (or might be hour:min??)
<01>|  <04><196>| set pump to rpm speed
<01>|  <04><228>| set pump to gpm speed
<04>|  <00>     | set pump control panel to local (local panel on)
<04>|  <255>    | set pump to remote control (local panel off)
<05>|  <00>     | set pump to filter 
<05>|  <01>     | set pump to manual
<05>|  <02>     | set pump to speed 1 
<05>|  <03>     | set pump to speed 2 
<05>|  <04>     | set pump to speed 3 
<05>|  <05>     | set pump to speed 4
<05>|  <06>     | set pump to feature 1
<06>|  <04>     | Turn pump off
<06>|  <10>     | Turn pump on
<07>|           | Request/Return Status
<09>|  <02><96> | run pump at <00><xx> xx gpm (from Intellitouch)


##  Pump Status Message

Request

Byte| 1  | 2|  3|   4|   5|  6 |
---|---|---|---|---|---|---|
Example  |96| 16| 7| 0| 1| 28|


Response

Byte|   1  | 2|  3|   4|   5|  6 | 7 | 8|    9| 10|   11| 12| 13| 14| 15| 16| 17|  18 | 19| 20|   21  
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|  
Example  |16| 96| 7| 15| 10| 0| 0| 0| 170| 4| 226| 0 |0 |0| 0| 0| 1| 22| 14| 2| 234|  

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Addresses](./Addresses) of message
3 | Y | Action | 7 = Status
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Mode/Command | 4=Off; 10=On
6 | N | Drivestate | Not sure
7 | Y | Watts High Bit | This bit * 256 + low bit = total watts
8 | Y | Watts Low Bit | 
9 | Y | RPM High Bit  | This bit * 256 + low bit = total RPM
10 | Y | RPM Low Bit
11 | N | PPC?  | Some control bit?
12 | N | ?
13 | N | Error(?) | Error message bit?
14 | N | Timer (?) | Some kind of timer, maybe?
15 | N | ?
16 | Y | Timer high bit | Either hh:mm (16:17) or mm:mm ? 
17 | Y | Timer low bit | How long before pump shuts off
18 | Y | Hours | Hour of actual time
19 | Y | Min | Mins of actual time
20 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
21 | Y | Checksum Low Bit

##  Set Speed Mode

Request

Byte| 1  | 2|  3|   4|   5|  6 | 7 | 8 | 9 | 10 | 
---|---|---|---|---|---|---|---|---|---|---|
Example  |96| 16|1| 4| 3| 39| 2| 238| 2| 52| 


Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Addresses](./Addresses) of message
3 | Y | Action | 1=Set speed mode
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Mode/Command | 3=Run program 
6 | Y | Action | 33=Run program XX/Turn off pump <BR> 39=Save Program 1 as XX Speed<BR> 40=Save Program 2 as XX Speed<BR>41=Prog3<BR> 42=Prog4<BR> 43=Set pump time for XX mins
7 | Y | High bit |  if save program, the high*256+low=RPM. <BR> If timer, hh:mm or mm:mm(?) in low bit & high bit.  
8 | Y | Low Bit | If Run then 0=off; 8=Prog1; 16=Prog2;24=Prog3;32=Prog4 (Low bit will be 0)
9 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
10 | Y | Checksum Low Bit


Response

Byte | 1 | 2|  3| 4| 5| 6 | 7| 8|  
---|---|---|---|---|---|---|---|---|
Example  |34| 96| 1| 2| 3| 232| 2| 21  

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Addresses](./Addresses) of message
3 | Y | Action | 1=Speed mode set
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Speed Mode High | (256)*this byte+byte 5
6 | Y | Speed Mode Low |  
7 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
8 | Y | Checksum Low Bit

##  Remote Control Mode

Request  

Byte|   1  | 2|  3|   4|   5|  6 | 7 |   
---|---|---|---|---|---|---|---|  
Example  |96| 16| 4| 1| 255| 2| 25|   

Response 
 
Byte|   1  | 2|  3|   4|   5|  6 | 7 |   
---|---|---|---|---|---|---|---|  
Example  |16| 96| 4| 1| 255| 2| 25  


Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Addresses](./Addresses) of message
3 | Y | Action | 4 = Pump Control Panel On\Off
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Mode/Command | 0=Set pump control panel (local) on; 255=Set pump control panel (local) off 
6 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
7 | Y | Checksum Low Bit



## Run Program X has a different type of response
```
16:55:52.137 VERBOSE Sent Packet 255,0,255,165,0,96,34,1,4,3,33,0,8,1,88 Try: 1
16:55:52.150 DEBUG Msg# 7  Msg received: 34,96,1,2,0,8,1,50
                           Msg written: 255,0,255,165,0,96,34,1,4,3,33,0,8,1,88
                           Match?: true
16:55:52.164 VERBOSE Msg# 7   Pump1: Speed Set to 8 rpm: [34,96,1,2,0,8,1,50]
```