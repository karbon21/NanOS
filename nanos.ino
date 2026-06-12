#include <WiFi.h>
//#include <FatFS.h>
//#include <FatFSUSB.h>
#include <XPT2046_Touchscreen.h>
#include "utils.h"
#include "keyboard.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define INDICATOR 20
#define TFT_CS 10
#define TFT_RST 11
#define TFT_DC 12
#define TOUCH_CS 13
#define SD_CS 17

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define LINE_CHARS 53

String version = "0.0.1";
String prompt = "> ";
double batteryCalibration = 1.01;

Keyboard keyboard;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

String inputBuffer = "";
bool inputMode = false;
std::vector<String> history;

unsigned long lastBlink = 0;
bool blink = false;

void print(String text, uint16_t color=ILI9341_WHITE) {
	tft.setTextColor(color, ILI9341_BLACK);
	for (int i = 0; i < text.length(); i++) {
        if (tft.getCursorY() >= SCREEN_HEIGHT) {
            tft.setCursor(0, 0);
            tft.fillRect(0, 0, SCREEN_WIDTH, CHAR_HEIGHT, ILI9341_BLACK);
        }

        if (tft.getCursorX() >= LINE_CHARS * CHAR_WIDTH) {
			if (tft.getCursorY() >= SCREEN_HEIGHT - CHAR_HEIGHT) {
				tft.setCursor(0, 0);
                tft.fillRect(0, 0, SCREEN_WIDTH, CHAR_HEIGHT, ILI9341_BLACK);
			} else {
            	tft.fillRect(0, tft.getCursorY() + CHAR_HEIGHT, SCREEN_WIDTH, CHAR_HEIGHT, ILI9341_BLACK);
			}
        }

        tft.print(text[i]);

        if (tft.getCursorX() <= 0) {
            tft.fillRect(0, tft.getCursorY(), SCREEN_WIDTH, CHAR_HEIGHT, ILI9341_BLACK);
        }
	}
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
}

float getBatteryVoltage() {
	return analogRead(26) * batteryCalibration * 3.3 * 2.0 / 65535.0;
}

float getBatteryPercentage() {
	float percentage = (getBatteryVoltage() - 3.0) / (4.2 - 3.0) * 100.0;
	if (percentage < 0) return 0;
	if (percentage > 100) return 100;
	return percentage;
}

void displayBlink(bool isBlinking) {
	int x = tft.getCursorX() + inputBuffer.length() * CHAR_WIDTH;
	int y = tft.getCursorY();
	uint16_t color = isBlinking ? ILI9341_WHITE : ILI9341_BLACK;
	tft.fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, color);
}

void displayInput(String input) {
	int ry = tft.getCursorY() + CHAR_HEIGHT * ceil((float) (input.length() + prompt.length()) / LINE_CHARS);
	tft.fillRect(0, ry, SCREEN_WIDTH, CHAR_HEIGHT, ILI9341_BLACK);
	int x = tft.getCursorX();
	int y = tft.getCursorY();
	tft.print(input);
	tft.setCursor(x, y);
}

void clear() {
	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(0, -CHAR_HEIGHT);
}

void connectToWiFi() {
    print("Connecting to WiFi", ILI9341_CYAN);
    WiFi.begin("Ucom9394_2.4G", "a1b2c3d4");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        print(".");
    }
    print("\nConnected!\n", ILI9341_GREEN);
    print("IP Address: ");
    print(WiFi.localIP().toString() + "\n");
}

void execute(String &input, bool isRepeated=false) {
	input.trim();
	if (input == "") return;
	if (!isRepeated) history.push_back(input);
	std::vector<String> args = splitString(input);
	input = "";
	String cmd = args[0];

	if (cmd == "help") {
		print("\n-- HELP --\n", tft.color565(0, 100, 200));
		print(" - a (...) - repeats the n-th command before this command, ");
		print("where n = the amount of arguments including initial \"a\".\n");
		print(" - help (page) - shows help page\n");
		print(" - me - shows information about this PC\n");
		print(" - cl - clears the screen.\n");
		print(" - bat - prints the current battery voltage and percentage.\n");
		print(" - uptime - shows the uptime in milliseconds since last reboot.\n");
		print(" - ping (address) - pings the given server.\n");
		print(" - draw - starts the graphical drawing program.\n");
		// print(" - msc - freezes the OS to enter file transfer mode.\n");
	} else if (cmd == "me") {
		print("NanOS version " + version + " on Raspberry Pi Pico 2 W.\n");
	} else if (cmd == "cl") {
		clear();
	} else if (cmd == "uptime") {
		print(String(millis()) + " milliseconds.\n");
	} else if (cmd == "a") {
		if (isRepeated) return;
		history.pop_back();
		int i = history.size() - args.size();
		if (i >= 0) {
			String command = history[i];
			execute(command, true);
		}
	} else if (cmd == "bat") {
		print("Voltage: ");
		print(String(getBatteryVoltage()), ILI9341_BLUE);
		print("V | Battery: ");
		print(String(getBatteryPercentage()), ILI9341_BLUE);
		print("%\n");
	} /*else if (cmd == "msc") {

	}*/ else if (cmd == "ping") {
		if (args.size() == 2) {
			if (WiFi.status() != WL_CONNECTED) connectToWiFi();
			WiFiClient client;
			print("Checking " + args[1] + "... ");
			if (client.connect(args[1].c_str(), 80)) {
				print("ONLINE\n", ILI9341_GREEN);
				client.stop();
			} else {
				print("OFFLINE/TIMEOUT\n", ILI9341_RED);
			}
		} else {
			print(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
			print("\n");
		}
	} else if (cmd == "draw") {
		uint16_t color = ILI9341_BLACK;
		int16_t size = 3;

		tft.fillScreen(ILI9341_WHITE);

		while (true) {
			if (ts.touched()) {
				TS_Point p = ts.getPoint();
				
				int x = map(p.x, 3900, 365, 0, SCREEN_WIDTH);
				int y = map(p.y, 3800, 200, 0, SCREEN_HEIGHT);
				
				tft.fillCircle(x, y, size, color);
			}

			char key = keyboard.getKey(true);
			if (!key) continue;
			if (key >= 'A' && key <= 'D') {
				size = key - 62;
			} else if (key == '*') {
				clear();
				break;
			} else {
				switch (key) {
					case '1':
						color = ILI9341_BLACK; break;
					case '2':
						color = ILI9341_BLUE; break;
					case '3':
						color = ILI9341_GREEN; break;
					case '4':
						color = ILI9341_CYAN; break;
					case '5':
						color = ILI9341_RED; break;
					case '6':
						color = ILI9341_MAGENTA; break;
					case '7':
						color = ILI9341_YELLOW; break;
					case '8':
						color = ILI9341_ORANGE; break;
					case '9':
						color = 0x9A60; break;
					case '0':
						color = ILI9341_WHITE; break;
				}
			}
		}
	}
}

void enterCommand() {
	displayBlink(false);
	print(inputBuffer + "\n");
	execute(inputBuffer);
	print("\n" + prompt);
}

void loop() {
	if (inputMode) {
		if (Serial.available() > 0) {
			String command = Serial.readString();
			inputBuffer = command;
			enterCommand();
		}

		char key = keyboard.getKey();
		if (key) {
			if (key == '\b') {
				if (inputBuffer.length() > 0) {
					inputBuffer.remove(inputBuffer.length() - 1);
					displayInput(inputBuffer + "  ");
				}
			} else if (key == '\n') {
				enterCommand();
			} else {
				inputBuffer += key;
			}
			displayInput(inputBuffer);
		}

		if (millis() - lastBlink > 500) {
			lastBlink = millis();
			blink = !blink;
			displayBlink(blink);
		}
	} else if (blink) {
		displayBlink(false);
	}
}

void setup() {
	pinMode(INDICATOR, GPIO_OUT);
	digitalWrite(INDICATOR, HIGH);

	analogReadResolution(16);
	analogRead(26);
	
	Serial.begin(115200);
	tft.begin();
	ts.begin();
	//FatFS.begin();
    //FatFSUSB.begin();

 	tft.setRotation(1);
  	tft.setTextSize(1);
	tft.fillScreen(ILI9341_BLACK);
	keyboard.getKey();

  	tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
	tft.println("\nWelcome to NanOS\n");
  	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

	tft.print(prompt);
	inputMode = true;
}