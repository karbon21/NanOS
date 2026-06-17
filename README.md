# NanOS
The OS for HackStation, a portable Raspberry Pi Pico 2 W device. NanOS features a command-line interface and touchscreen drawing support.

## Notice
THIS PROJECT IS IN EXPERIMENTAL STAGE, MANY FEATURES MAY BE BROKEN AND IT'S NOT YET SUITABLE FOR PRODUCTION.

## Features
* Keyboard Support: Full input handling via keypad or serial.
* Terminal Interface: Built-in shell for system commands.
* Battery Management: Battery voltage detection and automatic user warnings.
* Connectivity: WiFi integration for network diagnostics.
* Graphical Drawing: Interactive touchscreen drawing program.

## Hardware Requirements
- Raspberry Pi Pico / Pico 2 (2 W or W recommended for full wireless compatability)
- ILI9341 TFT Display
- XPT2046 Touchscreen Controller (Optional)
- External Keypad (Optional)
- 18650 Battery (Optional)

## Getting Started
1. Install the required Arduino libraries: Adafruit_GFX, Adafruit_ILI9341, and XPT2046_Touchscreen.
2. Upload the code via Arduino IDE.
3. Once booted, the terminal will appear. Start by typing help to explore the environment.