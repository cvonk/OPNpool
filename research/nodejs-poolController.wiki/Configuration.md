# Configuration

It always seemed that there should be a way for the Main controller to tell the remote controls/wireless/etc what the pool configuration is.  Sure enough, if any material changes (change temperature, circuit names, circuit configuration, schedules, egg timers, etc the Screenlogic Wireless unit will re-request all the status (aka it will be rebroadcast to every device).  

### 1. Date/Time

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request [Get Date/Time](Broadcast/#get-requests)|16|34|197|1|0|1|167|
Response [Date/Time](Broadcast/#time-clock-broadcast)|15|16|5|8|19|45|4|31|5|16|0|1|1|84

##  Set Date/Time

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
Ex|15| 16|  133| 8| 15| 34| 1| 10| 7| 16|  0| 1| 1| 47


Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](#actions) | 133 = Set Time/Clock
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Hour | 0=12am -> 23=11pm
6 | Y | Min |
7 | Y | Day of week| (Sun=1, Mon=2, Tue=4, Wed=8, Thu=16, Fri=32, Sat=64)
8 | Y | Day | Day # in month
9 | Y | Month  | 
10 | Y | Year | 20xx
11 | N | Clock Adjust | ?
12 | Y | DST | (1=Auto, 0=Manual)
13 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
14 | Y | Checksum Low Bit


### 2. Get Pump Configuration

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 19| 20| 21| 22| 23| 24| 25| 26| 27| 28| 29| 30| 31| 32|33|34|35|
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|216|1|1|1|187
Response|15|16|24|31|1|128|0|2|0|1|6|2|12|4|9|11|7|6|8|128|8|132|3|15|5|3|214|128|46|108|2|152|232|220|232|8|38|
|...
Request|16|34|216|1|2|1|188|
Response|15|16|24|31|2|128|3|2|0|12|3|5|5|13|7|14|11|0|3|0|3|0|3|0|3|12|232|220|208|184|232|232|232|232|28|8|242|


### 3 Request for Software Version

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 19| 20| 21| 22| 23| 24|  32|33|34|35|
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|253|1|0|1|223|
Response|15|16|252|17|0|2|90|0|0|1|10|0|0|0|0|0|0|0|0|0|0|2|66

### 4 Request for High Speed Circuits for Valves
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|222|1|0|1|192|
Response|15|16|30|16|0|0|0|0|1|72|0|0|0|46|0|0|0|2|0|0|1|117|

### 5 Request for Valve Status

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|221|1|0|1|191|
Response|15|16|29|24|2|0|0|0|128|1|255|255|255|0|7|2|1|8|5|6|7|8|9|10|1|2|3|4|4|204

### 6 Request for is4/is10 Spa Side Settings

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|224|1|0|1|194|
Response|15|16|32|11|0|7|2|1|8|5|6|7|8|9|10|1|56|

### 7 Request for Intelliflo Spa Side Control
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|214|1|0|1|184|
Reponse|15|16|22|16|0|2|0|0|0|1|50|10|1|144|13|122|15|130|0|0|2|220|

### 8 Request for Solar/Heat Pump Status
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|226|1|0|1|196|
Response|15|16|34|3|7|128|0|1|122

### 9 Request for Intelliflo Spa Side Remote settings
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|225|1|0|1|195|
Response|15|16|33|4|1|2|3|4|0|253|

### 10 Request for Delay Status
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|227|1|0|1|197|
Response|15|16|35|2|16|0|1|3

### 11 Request for Settings/Heat Mode
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|232|1|0|1|202|
Response|15|16|40|10|0|0|0|254|1|0|0|0|0|0|1|255


### 12 Custom Names 1-10
These are the custom names that can be configured from the screenlogic interface.  Bits 6+ are the Ascii characters.

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 19| 20|
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|202|1|0|1|181|
Response|15|16|10|12|0|85|83|69|82|78|65|77|69|45|48|49|3|219|||||
Custom Name 1||||||U|S|E|R|N|A|M|E|-|0|1
|||||| 0..9
Request|16|34|202|1|9|1|181|
Response|15|16|10|12|9|85|83|69|82|78|65|77|69|45|49|48|3|219|||||
Custom Name 10||||||U|S|E|R|N|A|M|E|-|1|0

#### Request

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](Broadcast/#actions) | 202 = Request Custom Name
4 | Y | 1 | Length of message
5 | Y | Number | Index or ID of custom name (0-9) 
6 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
7 | Y | Checksum Low Bit

#### Response

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](Broadcast/#actions) | 10 = Send Custom Name
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Number | Index or ID of custom name (0-9) 
6 | Y | Char1 | Ascii value of custom name digit 1
7-16| Y | Char2-11 | Ascii value of custom name digit 2-11
17 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
18 | Y | Checksum Low Bit

### 13 Circuit Names
The 7th bit refers to the list of [circuit names](Circuit-Names).  The ID #'s 200-209 refer to the [custom names](#3.-Custom-Names-1-10).

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11|
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|203|1|1|1|174|
Response|15|16|11|5|1|1|72|0|0|1|40|
|||||| 1..20
Request|16|34|203|1|20|1|193|
Response|15|16|11|5|20|0|93|0|0|1|79



#### Request

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](Broadcast/#actions) | 203 = Request Name
4 | Y | 1 | Length
5 | Y | Number | Index or ID of name (1-20) 
6 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
7 | Y | Checksum Low Bit

#### Response

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](Broadcast/#actions) | 11 = Send Name
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | Number | Index or ID of custom name (1-20) 
6 | Y | Function | [Circuit Function](Circuit-Names#Functions) (eg Feature, color wheel, spa, etc).  128 appears to indicate a macro.
7 | Y | Name | ID for a [list of names](Circuit-Names)
8 | N | Unknown/future?
9 | N | Unknown/future?
10 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
11 | Y | Checksum Low Bit

### 14 Schedules

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14|
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|209|1|1|1|180|
Response|15|16|17|7|1|6|9|29|15|55|255|2|88|
||||||||9:29||15:55||Every Day
||||||1..12
Request|16|34|209|1|12|1|191|
Response|15|16|17|7|12|5|13|30|13|40|145|1|232|
|||||||Spa|1:30pm||1:40pm||Tu, Thu

#### Request

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](Broadcast/#actions) | 209 = Get Schedule
4 | Y | 1 | Length
5 | Y | Schedule # | Schedule 1-12 
6 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
7 | Y | Checksum Low Bit

#### Response

Byte | Known? | Definition | Details
---: | ------ | ------------- | -------------
1 | Y | Destination | Destination [Address](./Addresses) of message 
2 | Y | Source Address | Sender [Address](./Addresses) of message
3 | Y | [Action](Broadcast/#actions) | 17 = Schedule (as previously saved)
4 | Y | Length | Number of bytes in the message after this byte
5 | Y | ID | Schedule ID/# (1-12) 
6 | Y | Circuit | Which circuit is the schedule on
7 | Y | Start Hour | If 25, then egg timer; if 1-24 then Start Hour
8 | Y | Start Min | Ignore if egg timer, else minute 
9 | Y | End Hour  | If egg timer, then # of hours.  Else end hour of schedule.
10 | Y | End Min | If egg timer, then # of minutes.  Else end minutes of schedule.
11 | Y | Days of week | Bitmask 0x0000000; 181 = Tu,Th,Fri,Sun; 145 = Thu,Sun; 255 = Every day;  Need to do more research but it seems like 0x01 is always active and 128=Sun; 16=Thu... need to just do some testing.
12 | Y | Checksum High Bit | This bit * 256 + low bit = checksum
13 | Y | Checksum Low Bit



### 15  Get Heat Set Points
Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 19| 20| 21| 22| 23| 24| 25| 26| 27| 28| 29| 30| 31| 32|33|34|35|
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|200|1|0|1|170|
Response [Heat/Temp](Broadcast#temperature-heat)|15|16|8|13|86|86|84|86|100|7|0|0|87|0|0|0|0|2|251|  


### 16 Request for Light Groups/Positions

Byte|1|2|3|4| 5| 6 | 7 | 8| 9| 10| 11| 12| 13| 14| 15| 16| 17|18 | 
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request|16|34|231|1|0|1|201|
Response|15|16|39|32|7|0|0|0|8|0|0|0|9|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|1|45