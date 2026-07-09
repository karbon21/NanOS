#include <WiFi.h>
#include <pico/cyw43_arch.h>

#include "utils.h"
#include "keyboard.h"
#include "jpg.h"

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

#include <FatFS.h>
#include <FatFSUSB.h>

#define TFT_CS 10
#define TFT_RST 11
#define TFT_DC 12
#define TOUCH_CS 13
#define SD_CS 17
#define INDICATOR 20
#define WL_CS 25
#define ADC_VSYS 29

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define LINE_CHARS 53

String version = "0.1.0";

Keyboard keyboard;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

String inputBuffer = "";
bool inputMode = false;
std::vector<String> history;

unsigned long lastBlink = 0;
bool blink = false;

double batteryCalibration;
String prompt;
String location;

void print(const String& text, uint16_t color=ILI9341_WHITE) {
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

String normalizePath(const String& path) {
	if (path.length() == 0) return "/";

    std::vector<String> parts;
    int startIndex = 0;

    while (startIndex < path.length()) {
        int slashIndex = path.indexOf('/', startIndex);
        String part;

        if (slashIndex == -1) {
            part = path.substring(startIndex);
            startIndex = path.length();
        } else {
            part = path.substring(startIndex, slashIndex);
            startIndex = slashIndex + 1;
        }

        if (part == "" || part == ".") {
            continue;
        } else if (part == "..") {
            if (!parts.empty()) {
                parts.pop_back();
            }
        } else {
            parts.push_back(part);
        }
    }

    String result = "";
    for (const String& p : parts) {
        result += "/" + p;
    }

    if (result == "") {
        return "/";
    }

    return result;
}

String joinPaths(const String& path1, const String& path2) {
	if (path1.isEmpty()) return path2;
    if (path2.isEmpty()) return path1;

    return normalizePath(path1 + "/" + path2);
}

String resolvePath(const String& path) {
	String resolved;

	if (path.startsWith("/")) {
		resolved = normalizePath(path);
	} else {
		resolved = joinPaths(location, path);
	}

	return resolved;
}

bool removeRecursive(String path) {
	path = normalizePath(path);
    Dir dir = FatFS.openDir(path);

    while (dir.next()) {
        String fileName = dir.fileName();
        String fullPath = joinPaths(path, fileName);

        if (dir.isDirectory()) {
            removeRecursive(fullPath); 
        } else {
            FatFS.remove(fullPath); 
        }
    }

    return FatFS.remove(path);
}

String configPath(String path) {
	return joinPaths("/system/config", path) + ".nconf";
}

bool configExists(String path) {
	String fullPath = configPath(path);
	return FatFS.exists(fullPath);
}

void setConfig(String path, String value, bool override=true) {
	if (configExists(path) && !override) return;
	String fullPath = configPath(path);
	File file = FatFS.open(fullPath, "w");
	file.print(value);
	file.close();
}

String getConfig(String path) {
	if (!configExists(path)) return "";
	String fullPath = configPath(path);
	File file = FatFS.open(fullPath, "r");
	String str = file.readString();
	file.close();
	return str;
}

float getBatteryVoltage() {
    cyw43_thread_enter();

    adc_gpio_init(ADC_VSYS);
    adc_select_input(3);
    gpio_put(WL_CS, true);

	delay(10);
    uint16_t raw = adc_read();

    gpio_set_function(ADC_VSYS, GPIO_FUNC_PIO1);
    gpio_init(WL_CS);
    gpio_set_dir(WL_CS, GPIO_OUT);
    gpio_put(WL_CS, false);

    cyw43_thread_exit();

	return (raw * 3.3 / 4095.0) * 3.0 * batteryCalibration;
}

float getBatteryPercentage(float voltage) {
	float percentage = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
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

void defaultConfigs() {
	setConfig("startup/delay", "200", false);
	setConfig("battery/calibration", "1", false);
	setConfig("terminal/prompt", "> ", false);
	setConfig("fs/home", "/user", false);
}

void verifyFilesystem() {
	std::array<String, 4> requiredDirectories = {"/system", "/system/config", "/system/stats", "/user"};

	for (String dir : requiredDirectories) {
		if (!FatFS.exists(dir)) {
			FatFS.mkdir(dir);
		}
	}

	Dir root = FatFS.openDir("/");
	while (root.next()) {
		String path = "/" + root.fileName();
        if (!contains(requiredDirectories, path)) {
			if (root.isFile()) {
				FatFS.remove(path);
			} else if (root.isDirectory()) {
				removeRecursive(path);
			}
        }
	}

	defaultConfigs();
}

bool connectToWiFi(String ssid, String passphrase) {
    WiFi.begin(ssid.c_str(), passphrase.c_str());
	int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
		count++;
		if (count > 10) {
			return false;
		}
    }
	setConfig("wifi/ssid", ssid);
	setConfig("wifi/pass", passphrase);
	return true;
}

bool connectToWiFi() {
	String ssid = getConfig("wifi/ssid");
	String pass = getConfig("wifi/pass");
	if (ssid == "") return false;
	return connectToWiFi(ssid, pass);
}

void help(int page) {
	print("\n-- HELP PAGE " + String(page) + " --\n", tft.color565(0, 100, 200));
	switch (page) {
		case 0:
			print(" - help (page) - shows help page (starting from 0).\n");
			print(" - me - displays information about this PC\n");
			print(" - cl - clears the screen.\n");
			print(" - bat - prints the current battery voltage and percentage.\n");
			print(" - upt - prints the uptime in milliseconds since last reboot.\n");
			print(" - loc - prints the current directory (location).\n");
			print(" - ls - lists the contents the current directory.\n");
			print(" - cd [directory] - changes the current directory.\n");
			print(" - a (...) - repeats the n-th command before this command, ");
			print("where n = the amount of arguments including initial \"a\".\n");
			print(" - rb - reboots NanOS.\n");
			break;
		case 1:
			print(" - rm [path] - recursively deletes a file or directory.\n");
			print(" - wifi (ssid) (passphrase) - connects to Wi-Fi.\n");
			print(" - ping (address) - pings the given server.\n");
			print(" - msc - freezes the OS to enter file transfer mode.\n");
			print(" - draw - starts the graphical drawing program.\n");
			print(" - jpg [path] - draws a JPG image on the screen.\n");
			break;
		default:
			print(" - help page not found.\n");
	}
}

void execute(String &input, bool isRepeated=false) {
	input.trim();
	if (input == "") return;
	if (!isRepeated) history.push_back(input);
	std::vector<String> args = splitString(input);
	input = "";
	String cmd = args[0];

	if (cmd == "help") {
		if (args.size() == 1) {
			help(0);
		} else {
			help(args[1].toInt());
		}
	} else if (cmd == "me") {
		print("NanOS version " + version + " on Raspberry Pi Pico 2 W.\n");
	} else if (cmd == "cl") {
		clear();
	} else if (cmd == "upt") {
		print(String(millis()) + " milliseconds.\n");
	} else if (cmd == "rb") {
		watchdog_reboot(0, 0, 0);
	} else if (cmd == "loc") {
		print(location + "\n");
	} else if (cmd == "ls") {
		Dir root = FatFS.openDir(location);
		while (root.next()) {
			print(root.fileName() + "\n");
		}
	} else if (cmd == "cd") {
		if (args.size() == 2) {
            String target = resolvePath(args[1]);
			
			if (target == "/" || FatFS.exists(target)) {
				location = target;
			}
		}
	} else if (cmd == "rm") {
		if (args.size() != 2) {
            print("Please specify the path of the file/folder to be removed.\n", ILI9341_RED);
        } else {
            String target = resolvePath(args[1]);

            if (!FatFS.exists(target)) {
                print("Error: Path not found.\n", ILI9341_RED);
            } else {
                print("Removing " + target + "...\n");

                if (removeRecursive(target)) {
                    print("Deleted successfully.\n", ILI9341_GREEN);
                } else {
                    print("Failed to delete.\n", ILI9341_RED);
                }
            }
		}
	} else if (cmd == "a") {
		if (isRepeated) return;
		history.pop_back();
		int i = history.size() - args.size();
		if (i >= 0) {
			String command = history[i];
			execute(command, true);
		}
	} else if (cmd == "bat") {
		float v = getBatteryVoltage();
		float p = getBatteryPercentage(v);
		print("Voltage: ");
		print(String(v), ILI9341_BLUE);
		print("V | Battery: ");
		print(String(p), ILI9341_BLUE);
		print("%\n");
	} else if (cmd == "msc") {
		print("Entering Mass Storage Class Mode...\n");
		delay(500);
    	FatFSUSB.begin();
		print("MSC Mode ON! After the work is done, press any key to reboot.", ILI9341_RED);
		while (true) {
			if (keyboard.getKey()) {
				FatFSUSB.end();
				watchdog_reboot(0, 0, 0);
			}
		}
	} else if (cmd == "wifi") {
		if (args.size() > 3) {
			print("Wrong amount of arguments.\n", ILI9341_RED);
		} else {
    		print("Connecting to WiFi...\n", ILI9341_CYAN);

			bool success;
			if (args.size() == 1) {
				success = connectToWiFi();
			} else {
				success = connectToWiFi(args[1], args.size() == 2 ? "" : args[2]);
			}
			
			if (success) {
				print("Connected!\n", ILI9341_GREEN);
				print("IP Address: ");
				print(WiFi.localIP().toString() + "\n");
			} else {
    			print("Failed!\n", ILI9341_RED);
			}
		}
	} else if (cmd == "ping") {
		if (args.size() == 2) {
			if (WiFi.status() != WL_CONNECTED) {
				if (!connectToWiFi()) {
					print("Connect to Wi-Fi first.\n", ILI9341_RED);
					return;
				}
			}
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
	} else if (cmd == "jpg") {
		if (args.size() != 2) {
            print("Please specify the path of the JPG file to be rendered.\n", ILI9341_RED);
        } else {
            String target = resolvePath(args[1]);

            if (!FatFS.exists(target)) {
                print("Error: Path not found.\n", ILI9341_RED);
            } else {
				tft.fillScreen(ILI9341_BLACK);
				drawJPG(target);

				while (true) {
					char key = keyboard.getKey(true);
					if (key == '*') {
						clear();
						break;
					}
				}
            }
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
	} else {
		print("Command not found!\n", ILI9341_RED);
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
			displayInput(inputBuffer);
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

void loop1() {
	float v = getBatteryVoltage();
	if (v != 0) {
		if (v < 3.0) {
			for (int i = 0; i < 100; i++) {
				digitalWrite(INDICATOR, LOW);
				delay(80);
				digitalWrite(INDICATOR, HIGH);
				delay(80);
			}
		}
		if (v < 3.2) {
			for (int i = 0; i < 10; i++) {
				digitalWrite(INDICATOR, LOW);
				delay(100);
				digitalWrite(INDICATOR, HIGH);
				delay(200);
			}
		} else if (v < 3.4) {
			for (int i = 0; i < 5; i++) {
				digitalWrite(INDICATOR, LOW);
				delay(500);
				digitalWrite(INDICATOR, HIGH);
				delay(1000);
			}
		}
	}
	delay(10000);
}

void setup() {
	pinMode(INDICATOR, GPIO_OUT);
	digitalWrite(INDICATOR, HIGH);

    adc_init();
	gpio_init(WL_CS);
	gpio_set_dir(WL_CS, GPIO_OUT);
	gpio_put(WL_CS, false);
	
	tft.begin();
	tft.setSPISpeed(50000000);
 	tft.setRotation(1);
  	tft.setTextSize(1);
	tft.fillScreen(ILI9341_BLACK);

	ts.begin();

	tft.println("Initializing Serial on 115200 baud rate");
	Serial.begin(115200);

	tft.println("Initializing Filesystem");
	FatFS.begin();

	tft.println("Verifying Filesystem");
	verifyFilesystem();

	tft.println("Loading Configurations");
	batteryCalibration = getConfig("battery/calibration").toDouble();
	prompt = getConfig("terminal/prompt");
	location = getConfig("fs/home");

	delay(getConfig("startup/delay").toInt());

	clear();
	print("\nWelcome to NanOS ", ILI9341_YELLOW);
	print("v" + version + "!\n\n", ILI9341_CYAN);

	keyboard.getKey();
	tft.print(prompt);
	inputMode = true;
}