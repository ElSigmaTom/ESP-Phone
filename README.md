# ESP-Phone

A simple way to turn two ESP8266 devices into interactive communication devices using ESP-NOW. This project includes a built-in web server to pre-make phrases and is built for the LCD1602 with an I2C backpack.

## Features
- Real-time communication between two ESP8266 devices
- Web server for pre-saving phrases
- LCD display for message viewing
- Built-in message saving and deletion
- Simple and intuitive user interface
- Efficient, low-power communication using ESP-NOW

## Planned Features
- [ ] **Retransmission**: If a message is sent and the peer is not within reach, the device will keep retransmitting the message until it is acknowledged (persists reboots).
- [ ] **LoRa**: LoRa for extended range.
- [ ] **Keypad_I2C**: Support for 4x3 Matrix Keypads via an I2C expander like PCF8574.

## Hardware Requirements
- Two ESP8266 devices
- LCD1602 with I2C backpack
- Push buttons (4)
- LEDs (2)

## Pinout
| Function         | Pin          |
|------------------|--------------|
| LCD SDA          | D1           |
| LCD SCL          | D2           |
| Display Button   | D3           |
| Up Button        | D5           |![pinout](https://github.com/user-attachments/assets/c636221f-1d09-4373-9c25-2ece67fb94c1)
| Down Button      | D6           |
| Enter Button     | D7           |
| Blue LED         | D8           |
| Red LED          | D4           |

## Getting Started

### Prerequisites
- ESP8266 board package installed in Arduino IDE
- [Arduino IDE](https://www.arduino.cc/en/software) installed
- [ESP8266WiFi](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi) library
- [ESP8266WebServer](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer) library
- [espnow](https://github.com/HarringayMakerSpace/ESP-Now) library
- [Wire](https://www.arduino.cc/en/Reference/Wire) library
- [LiquidCrystal_PCF8574](https://github.com/mathertel/LiquidCrystal_PCF8574) library
- [FS](https://github.com/esp8266/Arduino/tree/master/libraries/FS) library

### To Get Started
<details>
  <summary>Device MAC Addresses</summary>
1: 0xA0, 0x20, 0xA6, 0x1A, 0xA3, 0x13 (AP1 TARGET MAC)

2: 0x8C, 0xCE, 0x4E, 0xCB, 0x0A, 0xF0 (AP2 TARGET MAC)
</details>

### Identify MAC Addresses
1. Upload the following code to your ESP8266 devices to identify their MAC addresses:
  ```cpp
  #include <ESP8266WiFi.h>

  void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
  }

  void loop() {
  }
  ```
2. Open the serial monitor: Tools > Serial Monitor or `Ctrl + Shift + M`
3. Copy the MAC address and take note.
  > _Tip: Label your devices to keep track of them._
4. Copy the MAC address of the peer device into `Line 68` of the main code file in this repository.
5. Change SSID and Password on lines `10 and 11`.
  > _Tip: Set a unique SSID for each device._

### Usage
1. Power on your ESP8266 devices.
2. Connect to the WiFi network created by the devices.
3. Access the web interface by going to `http://192.168.4.1`.
4. Use the web interface to save or send messages.

### Troubleshooting
1. Ensure both devices are powered on and within range.
2. Check the serial monitor for error messages.
3. Verify that the MAC addresses are correctly configured.

## License
This project is licensed under the MIT License - see the [LICENSE](https://www.gnu.org/licenses/gpl-3.0.en.html) file for details.

## Acknowledgments
- Rui Santos & Sara Santos - [Random Nerd Tutorials](https://RandomNerdTutorials.com)

---

Happy building!


   



