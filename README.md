# ESP-Phone
## A simple way to turn two ESP8266's into interactive communication devices. Has built-in webserver to premake phrases. Built for LCD1602 w/ I2C backpack. 

### To get started:
Two devices minimum.
<sub> For Rafty:
(If the message is not received, consider swapping between these MAC addresses)
1: A0:20:A6:1A:A3:13 (AP1 TARGET MAC)
0xA0, 0x20, 0xA6, 0x1A, 0xA3, 0x13
2: 8C:CE:4E:CB:0A:F0 (AP2 TARGET MAC)
0x8C, 0xCE, 0x4E, 0xCB, 0x0A, 0xF0 
</sub>

1. First identify each mac address using the code below:
```
/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/get-change-esp32-esp8266-mac-address-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/
#include <ESP8266WiFi.h>

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}
 
void loop(){

}
```
2. Open serial monitor, click Tools > Serial Monitor or `crtl + shift + m`
3. Copy the mac address and take note.
_Note: I taped and wrote "I" and "II" on the backs of my devices to keep track._
4. Copy the mac address of the peer into `Line 68` of the code.
   
