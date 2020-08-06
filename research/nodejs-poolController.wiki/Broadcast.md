# Broadcast messages

The primary bit to look at when deciding what type of message is being broadcast is the Action/Command bit.  
For this, we know:

### Actions
```
        1: 'Ack Message',
        2: 'Controller Status',
        5: 'Date/Time',
        7: 'Pump Status',
        8: 'Heat/Temperature Status',
        10: 'Custom Names',
        11: 'Circuit Names/Function',
        16: 'Heat Pump Status?',
        17: 'Schedule details',
        18: 'IntelliChem',
        19: 'Intelli(?)',
        22: 'Get Intelliflo Spa Side Control',
        23: 'Pump Status',
        24: 'Pump Config',
        25: 'IntelliChlor Status',
        27: 'Pump Config (Extended)',
        29: 'Valve Status',
        30: 'High Speed Circuits for Valves',
        32: 'is4/is10 Settings',
        33: 'Intelliflo Spa Side Remote settings',
        34: 'Solar/Heat Pump Status',
        35: 'Delay Status',
        39: 'Light Groups/Positions',
        40: 'Settings, Heat Mode?',  //


        // Set commands
        96: 'Set Color', //Intellibrite, maybe more?
        131: 'Set Delay Cancel',
        133: 'Set Date/Time',
        134: 'Set Circuit',
        136: 'Set Heat/Temperature',
        138: 'Set Custom Name',
        139: 'Set Circuit Name/Function',
        144: 'Set Heat Pump',
        145: 'Set Schedule',
        146: 'Set IntelliChem',
        147: 'Set Intelli(?)',
        150: 'Set Intelliflow Spa Side Control',
        152: 'Set Pump Config',
        153: 'Set IntelliChlor',
        155: 'Set Pump Config (Extended)',
        157: 'Set Valves',
        158: 'Set High Speed Circuits for Valves',  //Circuits that require high speed
        160: 'Set is4/is10 Spa Side Remote',
        161: 'Set QuickTouch Spa Side Remote',
        162: 'Set Solar/Heat Pump',
        163: 'Set Delay',
        167: 'Set Light Groups/Positions',
        168: 'Set Heat Mode',  //probably more

        // Get commands
        194: 'Get Status/',
        197: 'Get Date/Time',
        200: 'Get Heat/Temperature',
        202: 'Get Custom Name',
        203: 'Get Circuit Name/Function',
        208: 'Get Heat Pump',
        209: 'Get Schedule',
        210: 'Get IntelliChem',
        211: 'Get Intelli(?)',
        214: 'Get Inteliflo Spa Side Control',
        215: 'Get Pump Status',
        216: 'Get Pump Config',
        217: 'Get IntelliChlor',
        219: 'Get Pump Config (Extended)',
        221: 'Get Valves',
        222: 'Get High Speed Circuits for Valves',
        224: 'Get is4/is10 Settings',
        225: 'Get Intelliflo Spa Side Remote settings',
        226: 'Get Solar/Heat Pump',
        227: 'Get Delays',
        231: 'Get Light group/positions',
        232: 'Get Settings, Heat Mode?',
        252: 'SW Version Info',
        253: 'Get SW Version'
```
There is a relationship between the various Status, Get, and Set messages.  The low order bits designate the type of message and the high order bits control whether or not you are requesting the current status or setting the current values.  For example the Date/Time message is type 5(00000101).  To request the Date/Time you would set the top two bits resulting in a type of 197(11000101).  To set the Date/Time you would set only the topmost bit resulting in a type of 133(10000101).  The same seems to apply to many of the other message types.

## Get Requests
All get requests are of the same form.  The action is set as defined above.  The payload is a single byte.  For simple requests that byte is 0, when requesting something that has more than one return value that byte is the index.

Byte|1|2|3|4|5|
---|---|---|---|---|---|
||16|32|action|1|index|
|Get Date/Time|16|32|197|1|0|
|Get Pump Status|16|32|215|1|0|
|Get Circuit 1 Name|16|32|203|1|1|
|Get Circuit 18 Name|16|32|203|1|18|

## Examples

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 19| 20| 21| 22| 23| 24| 25| 26| 27| 28| 29| 30| 31| 32|33|34|35|
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
[Equipment/Status](#equipment-status) |15| 16|  2| 29| 11| 33| 32|  0|  0|  0|  0|  0|  0|  0| 51|  0| 64|  4| 79| 79| 32|  0| 69|102|  0|  0|  7|  0|  0|182|215|  0| 13|  4|186
[Time/Clock](#time-clock-broadcast) |15| 16|  5| 8| 15| 34| 1| 10| 7| 16|  0| 1| 1| 47
[Temp/Heat](#temperature-heat)|15| 16|  8| 13| 85| 85| 73| 87| 95| 3|  0| 0| 104 | 0|0|0|0|2|247


## Equipment Status 

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 19| 20| 21| 22| 23| 24| 25| 26| 27| 28| 29| 30| 31| 32|33|34|35|
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Ex|15| 16|  2| 29| 11| 33| 32|  0|  0|  0|  0|  0|  0|  0| 51|  0| 64|  4| 79| 79| 32|  0| 69|102|  0|  0|  7|  0|  0|182|215|  0| 13|  4|186

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](#actions) | 2 = Circuit/Equipment status(?); 5 = Clock/Time; 8 = Temp/Heat(?)
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Hour | 
6 | Y | Min |
7 | Y | Equipment 1-8 | The first 8 physical circuits [See circuit bitmask](misc/#circuit-bitmask)
8 | Y | Features 1-8 | Physical circuits 9-16 [See circuit bitmask](misc/#circuit-bitmask)
9 | Y | Features 9-16  | Physical circuits 17-23, Aux X (X is available if solar is not used) [See circuit bitmask](misc/#circuit-bitmask)
10 | Y | Features 17-24
11 | Y | Features 25-32
12 | Y | Features 33-40
13 | Y | Features 41-50(?)
14 | Y | Mode/Units/Etc | Each bit has its own meaning when set or not.  At present not all bits have been decoded.  (Off[0]/On[1])<br>1 - Run Mode: Auto/Service<br>4 - Temp Units: Fahrenheit/Celsius<br>8 - Freeze Protection: Off/On<br>128 - Timeout: Off/On
15 | N | ?
16 | N | ?
17 | N | ?
18 | N | ?
19 | Y | Water Temp
20 | N | Temperature 2 | ?
21 | N | Valves
22 | N |
23 | Y | Air Temp
24 | Y | Solar Temp
25 | N |
26 | N |
27 | Y | [Pool/Spa Heat Mode](Misc/#spa-pool-heat-mode)
28 | N |
29 | N |
30 | N |
31 | N |
32 | N |
33 | N |
34 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
35 | Y | Checksum Low Bit


 
##  Time Clock Broadcast

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Ex|15| 16|  5| 8| 15| 34| 1| 10| 7| 16|  0| 1| 1| 47


Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](#actions) | 5 = Time/Clock Broadcast
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Hour | 
6 | Y | Min |
7 | Y | Day of week| (Sun=1, Mon=2, Tue=4, Wed=8, Thu=16, Fri=32, Sat=64)
8 | Y | Day | Day # in month
9 | Y | Month  | 
10 | Y | Year | 20xx
11 | N | Clock Adjust | ?
12 | Y | DST | (1=Auto, 0=Manual)
13 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
14 | Y | Checksum Low Bit


##  Set Pool/Spa Heat

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 
---|---|---|---|---|---|---|---|---|---|---|
Ex|116| 34|  136| 4| 87| 100| 15| 0| 2| 55|


Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](#actions) | 5 = Time/Clock Broadcast
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Pool Heat Set point | 
6 | Y | Spa Heat Set point | See [Miscellaneous](./Misc)
7 | Y | Pool/Spa Heat Mode
8 | N | 0 | Is this always 0?
9 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
10 | Y | Checksum Low Bit


##  Temperature Heat

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15|16|17|18|19
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Ex|15| 16|  8| 13| 85| 85| 73| 87| 95| 3|  0| 0| 104 | 0|0|0|0|2|247

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](#actions) | 8 = Temperature/Heat Status
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Temp1 | 
6 | Y | Temp2 |
7 | Y | Air Temp| 
8 | Y | Pool Setpoint
9 | Y | Spa Setpoint  | 
10 | Y | [Pool/Spa Heat Mode](Misc/#spa-pool-heat-mode) | 2 bits for each (0=Off, 1= Heater, 2=Solar Preferred, 3=Solar Only)
11 | N | 
12 | N | 
13 | Y | Solar Temp 
14 | N | 
15 | N | 
16 | N | 
17 | N | 
18 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
19 | Y | Checksum Low Bit

## Software Version
Byte|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Ex|15|16|252|17|0|2|100|0|0|1|10|0|0|0|0|0|0|0|0|0|0|2|76|

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](#actions) | 252 = SW Version
4 | Y | Length | Number of bytes in the message after this byte
5 | N |
6 | Y | SW Major Version
7 | Y | SW Minor Version (%03d)
8 | N |
9 | N | 
10 | Y | Bootloader Major Version
11 | Y | Bootloader Minor Version (%03d)
12 | N | 
13 | N | 
14 | N | 
15 | N | 
16 | N |
17 | N |
18 | N |
19 | N |
20 | N |
21 | N |
22 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
23 | Y | Checksum Low Bit