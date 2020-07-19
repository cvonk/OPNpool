#### This page is for the Pentair (aka AquaRite, NatureSoft, MineralSprings, SmartPure, etc) Salt Chlorinator


# Packets

These packets are a little different than all of the others in this wiki.  They all start with 16,2 and end with 16,3.  There is no high bit checksum (only a low bit).

## Wiring

If you are wiring this yourself, the pinouts are also slightly different than the other pool peripherals.

```
Aquarite pins:
1=Red=Power
2=Blk = data +
3=Ylw= data -
4=Grn=Ground
```

## 0. List of Commands
Command | Name |
----: | ---------------- |
0 | Get Status 
1 | Response to Get Status
3 | Response to Get Version
17 | Set Salt %
18 | Response to Set Salt % & Salt PPM
20 | Get Version
21 | Set Salt Generate % / 10


## 1. Status Request 

Byte|1|2|3|4| 5| 6 | 7 | 8| 9|  
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request |16|2|80|0|0|98|16|3|
Response |16|2|0|1|0|0|19|16|3|
  

#### Request

Byte | Known? | Sample | Definition 
---: | ------ | -----: | -------------
1 | Y | 16 | Preamble 1 
2 | Y | 2 | Preamble 2
3 | Y | 80 | Destination = Chlorinator
4 | Y | 0 | Command = Get Status
5 | N | 0 | Not sure 
6 | Y | 98 | Checksum Low Bit | This bit is checksum of 1-5 bits (MOD 256, if necessary)
7 | Y | 16 | Post-amble 1
8 | Y | 3 | Post-amble 2

#### Response

Byte | Known? | Sample | Definition 
---: | ------ | -----: | -------------
1 | Y | 16 | Preamble 1 
2 | Y | 2 | Preamble 2
3 | Y | 0 | Destination = Controller
4 | Y | 1 | Status = Ok
5 | N | 0 | Not sure 
6 | N | 0 | Not sure
7 | Y | 19 | Checksum Low Bit | This bit is checksum of 1-6 bits (MOD 256, if necessary)
8 | Y | 16 | Post-amble 1
9 | Y | 3 | Post-amble 2

## 2. Set Generate Salt % / Return Salt PPM

Byte|1|2|3|4| 5| 6 | 7 | 8| 9|  
---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
Request |16|2|80|17|3|118|16|3|   
Response |16|2|0|18|58|144|238|16|3|
  

#### Request

Byte | Known? | Sample | Definition 
---: | ------ | -----: | -------------
1 | Y | 16 | Preamble 1 
2 | Y | 2 | Preamble 2
3 | Y | 80 | Destination = Chlorinator
4 | Y | 17 | Command = Set Salt % <br> If this is 21, then bit 5 = Set Salt %/10.  
5 | Y | 3 | Set Salt % (3%) <br> If bit 4 is 21, then divide this by 10 (EG 100/10=10%);
6 | Y | 118 | Checksum Low Bit, This bit is checksum of 1-5 bits (MOD 256, if necessary)
7 | Y | 16 | Post-amble 1
8 | Y | 3 | Post-amble 2

#### Response

Byte | Known? | Sample | Definition 
---: | ------ | -----: | -------------
1 | Y | 16 | Preamble 1 
2 | Y | 2 | Preamble 2
3 | Y | 0 | Destination = Controller
4 | Y | 18 | Status = Salt PPM Command
5 | Y | 58 | Salt PPM * 50 (58*50=2,900PPM) 
6 | Y | 144 | Error bit:  0=Ok, 1=No Flow, 2=Low Salt, 4=High Salt, 144=Clean Salt Cell
7 | Y | 238 | Checksum Low Bit | This bit is checksum of 1-6 bits (MOD 256, if necessary)
8 | Y | 16 | Post-amble 1
9 | Y | 3 | Post-amble 2

## 2. Get Version / Return Name

Byte|   1  | 2|  3|   4|   5|  6 | 7 | 8|    9| 10|   11| 12| 13| 14| 15| 16| 17|  18 | 19| 20|   21  |22|23|24|
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|  
Request |16|2|80|20|2|120|16|3|   
Response |16|2|0|3|0|73|110|116|101|108|108|105|99|104|108|111|114|45|45|52|48|188|16|3|
  

#### Request

Byte | Known? | Sample | Definition 
---: | ------ | -----: | -------------
1 | Y | 16 | Preamble 1 
2 | Y | 2 | Preamble 2
3 | Y | 80 | Destination = Chlorinator
4 | Y | 20 | Command = Get Name
5 | N | 2 | Not sure.  Intellitouch uses 2.  Aquarite uses 0.  Any of them seem to work. 
6 | Y | 120 | Checksum Low Bit, This bit is checksum of 1-5 bits (MOD 256, if necessary)
7 | Y | 16 | Post-amble 1
8 | Y | 3 | Post-amble 2

#### Response

Byte | Known? | Sample | Definition 
---: | ------ | -----: | -------------
1 | Y | 16 | Preamble 1 
2 | Y | 2 | Preamble 2
3 | Y | 0 | Destination = Controller
4 | Y | 3 | Command = Return Name
5 | Y | 58 | Salt PPM * 50 (58*50=2,900PPM) 
6-21 | Y | many | Ascii bits.  This example returns "Intellichlor--40"  
22 | Y | 188 | Checksum Low Bit | This bit is checksum of 1-21 bits (MOD 256, if necessary).  Here the sum is 1468.  1468%256=188.
23 | Y | 16 | Post-amble 1
24 | Y | 3 | Post-amble 2