# NanOS
The OS for HackStation, a portable Raspberry Pi Pico 2 W device. NanOS features a command-line interface and touchscreen support.

## Notice
THIS PROJECT IS IN EXPERIMENTAL STAGE, MANY FEATURES MAY BE BROKEN AND IT'S NOT YET SUITABLE FOR PRODUCTION.

## Features
* Input handling via keypad or serial
* Interactive shell for system commands
* Battery management for safety
* Persistent filesystem for NanOS system configuration files and user data
* Built-in text editor (NEdit)
* WiFi integration (For Pico W/2W models)

## Hardware Requirements
- Raspberry Pi Pico / Pico 2 (2 W or W recommended for wireless compatability)
- ILI9341 TFT Display
- XPT2046 Touchscreen Controller (Recommended)
- External Keypad (Optional)
- LED Indicator + 470 Ohm Resistor wired to pin 20 (Optional)
- 18650 Battery (Optional)
- TP4056 Charging Board (Recommended if using a battery)

## Getting Started
### Install the latest version
1. Download the .uf2 file from the latest release.
2. Plug in your Pico while holding BOOTSEL.
3. Drag and drop the .uf2 file into the USB drive that appears.

### Or build the project in Arduino IDE yourself
1. Download the source code from GitHub.
2. Download and install Arduino IDE.
3. Install Earle Philhower's Arduino-Pico Core.
4. Install the required Arduino libraries: Adafruit_GFX, Adafruit_ILI9341, XPT2046_Touchscreen, TJpg_Decoder.
5. Change FS size (Tools -> Flash Size) to as much as you can, while leaving at least 1MB for Sketch.
6. Upload the code via Arduino IDE.

Once booted, the terminal will appear. Start by typing help to explore the environment.

## 🎉 Bonus Features 🎉
* Screen brightness control (Connect TFT's LED pin to Pico's pin 21)
* A built-in interactive drawing program
* Built-in support for JPG images
* JPG sequence animation support
* File transfer mode compatible with Windows, MacOS and Linux
