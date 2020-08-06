# Miscellaneous 

### Circuit bitmask
The Pentair bus sends many packets in bitmask form.  One of these you will find is the [Circuits/Features](Broadcast#equipment-status) bits 7, 8, 9.
Each binary bit is an on/off signal for a circuit.  
0x0000001 = 1 = Only 1st circuit is on  
0x0000010 = 2= Only 2nd circuit is on  
...  
0x0010101 = 21 = Circuits 1, 3, and 5 are on  

Bit 7 is for circuits 1-8, Bit 8 is for 9-16 and Bit 9 is for 17-24.

### Spa Pool Heat Mode
Seen in Action = 8 [Temp/Heat status](Broadcast/#temperature-heat) and Action = 
Pentair controller sends the pool and spa heat status as a 4 digit binary byte from 0000 (0) to 1111 (15).  The left two (xx__) is for the spa and the right two (__xx) are for the pool.  EG 1001 (9) would mean 10xx = 2 (Spa mode Solar Pref) and xx01 = 1 (Pool mode Heater)


### Day of week
In the [Time Clock Broadcast](Broadcast/#time-clock-broadcast) this is 
(Sun=1, Mon=2, Tue=4, Wed=8, Thu=16, Fri=32, Sat=64)
Sunday = 0x0000001  
Monday = 0x0000010  
Tuesday = 0x0000100  
Wednesday = 0x0001000  
Thursday = 0x0010000  
Friday = 0x0100000  
Saturday = 0x1000000  

However, this is different in the [Configuration schedules](Configuration/#schedules)
Bitmask 0x0000000; 181 = Tu,Th,Fri,Sun; 145 = Thu,Sun; 255 = Every day; Need to do more research but it seems like 0x01 is always active and 128=Sun; 16=Thu... need to just do some testing.