# DIY Pip-Boy 3000 Mark IV

A fully functional Pip-Boy 3000 Mark IV replica inspired by the Fallout series. This project combines a digital clock, environmental sensors, and internet radio functionality into a retro-futuristic wearable device.

![IMG_6981](https://github.com/user-attachments/assets/6239cd6a-6301-482b-a73d-d807f1951759)

# You can get the files and instructions (Diagrams) from: https://www.thingiverse.com/thing:6654866

## Features

### 🕒 STAT Menu - Time Display
- 12-hour digital clock format with flashing colon
- Dynamic time period indicators:
  - Morning (5:00 AM - 11:59 AM)
  - Afternoon (12:00 PM - 4:59 PM)
  - Evening (5:00 PM - 8:59 PM)
  - Night (9:00 PM - 4:59 AM)
- Animated Vault Boy display

### 📊 DATA Menu - Environmental Monitoring
- Temperature display in Fahrenheit
- Humidity sensor readings
- Retro-style UI with Fallout-inspired graphics
- Radiation meter animation

### 📻 RADIO Menu
- Internet radio functionality with three preset stations:
  - 60s and 70s Hits
  - 70s Greatest Hits
  - Q102 80s
- Song information display (artist and title)
- MP3 player support via DFPlayer Mini
- Custom sound effects for menu navigation

## Hardware Requirements

- ESP32 board - https://www.aliexpress.us/item/3256806034074322.html?spm=a2g0o.order_list.order_list_main.119.43681802dTr0Q7&gatewayAdapt=glo2usa
  
- TFT Display - https://www.aliexpress.us/item/3256806796021810.html?spm=a2g0o.order_list.order_list_main.129.43681802dTr0Q7&gatewayAdapt=glo2usa
  
- AHT20/AHT10 Temperature & Humidity Sensor - https://www.aliexpress.us/item/3256806761729546.html?spm=a2g0o.order_list.order_list_main.114.43681802dTr0Q7&gatewayAdapt=glo2usa
  
- DFPlayer Mini MP3 Player -  https://www.aliexpress.us/item/3256807449843805.html?spm=a2g0o.order_list.order_list_main.64.43681802dTr0Q7&gatewayAdapt=glo2usa

- PCM5102A DAC (for internet radio) - https://www.aliexpress.us/item/3256807291175573.html?spm=a2g0o.order_list.order_list_main.69.43681802dTr0Q7&gatewayAdapt=glo2usa

- Rotary switch - https://www.aliexpress.us/item/2255800513929044.html?spm=a2g0o.order_list.order_list_main.94.43681802dTr0Q7&gatewayAdapt=glo2usa

- speaker - https://www.aliexpress.us/item/3256806810941225.html?spm=a2g0o.order_list.order_list_main.59.43681802dTr0Q7&gatewayAdapt=glo2usa

- Leds - https://www.aliexpress.us/item/3256806034354191.html?spm=a2g0o.order_list.order_list_main.74.43681802dTr0Q7&gatewayAdapt=glo2usa

- 510 Ohm Resistors - https://www.aliexpress.us/item/3256806678673960.html?spm=a2g0o.order_list.order_list_main.89.43681802dTr0Q7&gatewayAdapt=glo2usa

- Dupont wires - https://www.aliexpress.us/item/3256803454873245.html?spm=a2g0o.order_list.order_list_main.83.43681802dTr0Q7&gatewayAdapt=glo2usa

  
- SD Card (for MP3 player)
  
- WiFi connection

### Pin Configuration
```
MP3 Player:
- RX: GPIO16
- TX: GPIO17

I2S DAC:
- BCLK: GPIO12
- LRC: GPIO13
- DOUT: GPIO14

Input Buttons:
- STAT: GPIO25
- INV: GPIO26
- DATA: GPIO27
- TIME: GPIO32
- MP3PLAYER: GPIO33
- RADIO: GPIO34
- RADIO1: GPIO35
- RADIO2: GPIO36
```

## Software Dependencies

- TFT_eSPI library
- WiFiManager
- NTPClient
- DFRobotDFPlayerMini
- Adafruit_AHTX0
- AnimatedGIF
- Audio libraries for ESP32 (for internet radio)

## Setup Instructions

1. Install required libraries through Arduino IDE
2. Configure your WiFi credentials:
```cpp
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
```

3. Set your timezone:
```cpp
int UTC = -8;  // Adjust according to your timezone
```

4. Configure notification volume (0-30):
```cpp
uint16_t notification_volume = 25;
```

5. Format SD card as FAT32 and add your MP3 files
6. Upload the code to your ESP32

## Usage

The Pip-Boy interface is navigated using the Rotary Switch:

- TIME: Display clock with animated Vault Boy
- DATA: Show temperature and humidity readings
- RADIO/RADIO1/RADIO2: Access different radio stations
- MP3PLAYER: Play local MP3 files
- STAT/INV: Display status animations

## Customization

You can customize various aspects:
- Add more radio stations in the `stations` array
- Modify color schemes using the defined color constants
- Adjust display positions and layout
- Add new animations or menu screens

## Debug Mode

Enable debugging by setting:
```cpp
#define DEBUG 1
```
This will output detailed information to the Serial Monitor at 115200 baud.

## Contributing

Feel free to contribute to this project:
1. Fork the repository
2. Create your feature branch
3. Submit a pull request

## Acknowledgments

- Inspired by Fallout's Pip-Boy 3000 Mark IV
- Built using the Arduino ecosystem for ESP32
- Utilizes various open-source libraries
- Special thanks to the Fallout modding community for inspiration
- Credit to: https://ytec3d.com/pip-boy-3000-mark-iv/
- Credit to: https://www.youtube.com/@jejelinge5978

## License

This project is licensed under the MIT License - see the LICENSE file for details.
