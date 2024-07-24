# ESP-Phone
A simple way to turn two ESP8266's into interactive communication devices. Has built-in webserver to premake phrases. Built for LCD1602 w/ I2C backpack. 

### To get started:
- Two devices minimum.



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
   
