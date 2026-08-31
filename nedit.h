#pragma once

#include <Arduino.h>
#include <FatFS.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>
#include "keyboard.h"
#include "config.h"
#include "const.h"

const String neditVersion = "1.0";

void save(const String& path, const String& fileContent) {
	File file = FatFS.open(path, "w");
	file.print(fileContent);
	file.close();
}

void printCursor(uint16_t color) {
	tft.fillRect(tft.getCursorX(), tft.getCursorY(), CHAR_WIDTH, CHAR_HEIGHT, color);
}

void runEditor(const String& path, Adafruit_ILI9341& tft, XPT2046_Touchscreen& ts, Keyboard& keyboard) {
	tft.fillScreen(ILI9341_BLACK);
	tft.fillRect(0, 0, SCREEN_WIDTH, CHAR_HEIGHT, ILI9341_BLUE);
	tft.setCursor(0, 0);
    tft.setTextColor(ILI9341_ORANGE, ILI9341_BLUE);
    tft.print("NEdit v");
	tft.print(neditVersion);
	tft.print(" - ");

	String fileContent = "";

    File file = FatFS.open(path, "r");
    if (file) {
        fileContent = file.readString();
        file.close();
		tft.print("Loaded File\n");
    } else tft.print("New File\n");
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
	tft.setCursor(0, CHAR_HEIGHT);
	tft.print(fileContent);
	tft.setTextWrap(false);

	bool editMode = false;
	bool initial = true;

	int prevX = 0;
	int prevY = 0;
	int scrollX = 0;
	int scrollY = 0;
	
	int left = getConfig("touch/left").toInt();
	int right = getConfig("touch/right").toInt();
	int top = getConfig("touch/top").toInt();
	int bottom = getConfig("touch/bottom").toInt();
	
	int length = fileContent.length();
	int index = length;

	while (true) {
		if (editMode) {
			if (ts.touched() || initial) {
				if (!initial) {
					TS_Point p = ts.getPoint();
					
					int x = map(p.x, left, right, 0, SCREEN_WIDTH);
					int y = map(p.y, top, bottom, 0, SCREEN_HEIGHT);

					if (x >= 10 && x <= 30 && y <= SCREEN_HEIGHT - 10 && y >= SCREEN_HEIGHT - 30) {
						bool draw = true;
						while (true) {
							if (draw) {
								tft.fillScreen(ILI9341_BLACK);
								tft.setCursor(0, 0);
								tft.setTextSize(3);

								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
								tft.print("Press ");
								tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
								tft.print("#");
								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
								tft.print(" to ");
								tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
								tft.print("save");
								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
								tft.println(".");

								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
								tft.print("Press ");
								tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
								tft.print("*");
								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
								tft.print(" to ");
								tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
								tft.print("cancel");
								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
								tft.println(".");

								tft.setTextSize(1);
								draw = false;
							}
							
							char key = keyboard.getKey(true);
							if (key == '#') {
								tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
								tft.setCursor(SCREEN_WIDTH / 2 - 6 * CHAR_WIDTH / 2, SCREEN_HEIGHT - CHAR_HEIGHT);
								tft.print("Saved!");
								tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

								save(path, fileContent);
								break;
							} else if (key == '*') break;
						}
					} else if (x >= SCREEN_WIDTH - 30 && x <= SCREEN_WIDTH - 10 && y <= SCREEN_HEIGHT - 10 && y >= SCREEN_HEIGHT - 30) {
						editMode = false;
						initial = true;
						continue;
					}

					if (prevX) scrollX += x - prevX;
					if (prevY) scrollY += y - prevY;

					if (scrollX > 0) scrollX = 0;
					if (scrollY > 0) scrollY = 0;

					prevX = x;
					prevY = y;
					
					int targetCol = (x - scrollX) / CHAR_WIDTH;
					int targetRow = (y - scrollY) / CHAR_HEIGHT;

					int currentCol = 0;
					int currentRow = 0;
					
					length = fileContent.length();
					index = length;

					for (int i = 0; i < length; i++) {
						if (fileContent[i] == '\n') {
							if (currentRow == targetRow) {
								index = i;
								break;
							}
							currentRow++;
							currentCol = 0;
						} else {
							if (currentCol == targetCol && currentRow == targetRow) {
								index = i;
								break;
							}
							currentCol++;
						}
					}
				}

				tft.fillScreen(ILI9341_BLACK);

				tft.setCursor(scrollX, scrollY);
				for (int i = 0; i < length; i++) {
					uint16_t color = i == index ? ILI9341_LIGHTGREY : ILI9341_BLACK;
					tft.setTextColor(ILI9341_WHITE, color);
					if (fileContent[i] == '\n') {
						printCursor(color);
						tft.setCursor(scrollX, tft.getCursorY() + CHAR_HEIGHT);
					}
					else tft.print(fileContent[i]);
				}
				if (index == length) printCursor(ILI9341_LIGHTGREY);
				tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

				tft.fillRect(10, SCREEN_HEIGHT - 30, 20, 20, ILI9341_GREENYELLOW);
				tft.fillRect(SCREEN_WIDTH - 30, SCREEN_HEIGHT - 30, 20, 20, ILI9341_RED);
				
				initial = false;
			} else {
				prevX = 0;
				prevY = 0;
			}

			char key = keyboard.getKey();
			if (key) {
				if (key == '\b') {
					if (index > 0) {
						fileContent = fileContent.substring(0, index - 1) + fileContent.substring(index, length);
						initial = true;
						length--;
						index--;
					}
				} else {
					fileContent = fileContent.substring(0, index) + key + fileContent.substring(index, length);
					initial = true;
					length++;
					index++;
				}
			}
		} else {
			tft.setTextColor(ILI9341_CYAN, ILI9341_DARKGREEN);
			tft.setCursor(SCREEN_WIDTH / 2 - 27 * CHAR_WIDTH / 2, SCREEN_HEIGHT / 2 - CHAR_HEIGHT);
			tft.println("Press 5 to enter edit mode.");
			tft.setCursor(SCREEN_WIDTH / 2 - 17 * CHAR_WIDTH / 2, SCREEN_HEIGHT / 2);
			tft.println("Press * to leave.");
			char key = keyboard.getKey(true);
			if (key == '5') editMode = true;
			else if (key == '*') {
				break;
			}
		}
	}
	tft.setTextWrap(true);
}