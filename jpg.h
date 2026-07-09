#pragma once

#include <Arduino.h>
#include <FatFS.h>
#include <Adafruit_ILI9341.h>
#include <TJpg_Decoder.h>

extern Adafruit_ILI9341 tft;

inline bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return 0;
    tft.drawRGBBitmap(x, y, bitmap, w, h);
    return 1;
}

inline void drawJPG(const String& filename, int x = 0, int y = 0) {
	TJpgDec.setCallback(tft_output);
    
    if (TJpgDec.drawFsJpg(x, y, filename.c_str(), FatFS) != 0) {
		tft.setCursor(0, 0);
        tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
        tft.println("Failed to open or decode JPG!");
    }
}