#include "AESYS9929B.h"

AESYS9929B::AESYS9929B(
	uint8_t width,
	uint8_t data_high,
	uint8_t clock_high,
	uint8_t data_low,
	uint8_t clock_low,
	uint8_t latch,
	uint8_t output_enable) :
	Adafruit_GFX(width, 16),
	PIN_DH(data_high),
	PIN_CH(clock_high),
	PIN_DL(data_low),
	PIN_CL(clock_low),
	PIN_LAT(latch),
	PIN_OE(output_enable) {
	BUFFER = (uint8_t*)calloc(width * 2, sizeof(uint8_t));
	pinMode(PIN_DH, OUTPUT);
	pinMode(PIN_CH, OUTPUT);
	pinMode(PIN_DL, OUTPUT);
	pinMode(PIN_CL, OUTPUT);
	pinMode(PIN_LAT, OUTPUT);
	pinMode(PIN_OE, OUTPUT);
	display();
	digitalWrite(PIN_OE, 0);
#ifdef ESP8266 //Limit PWM range to 8 bit as on AVRs
	analogWriteRange(255);
#endif
}

void AESYS9929B::drawPixel(int16_t x, int16_t y, uint16_t color) {
	if (y < 0 || y >= height() || x < 0 || x >= width()) return;
	uint16_t b = x;
	if (y >= 8) b += width();
	if (color) {
		BUFFER[b] |= 1 << (y & 7);
	} else {
		BUFFER[b] &= 0xFF ^ (1 << (y & 7));
	}
}

void AESYS9929B::fillScreen(uint16_t color) {
	for (uint16_t x = 0; x < width(); x++) {
		if (color) {
			BUFFER[x] = 0xFF;
			BUFFER[x + width()] = 0xFF;
		} else {
			BUFFER[x] = 0;
			BUFFER[x + width()] = 0;
		}
	}
}

void AESYS9929B::display() {
	for (uint16_t x = 0; x < width(); x++) {
		shiftOut(PIN_DH, PIN_CH, LSBFIRST, BUFFER[x]);
		shiftOut(PIN_DL, PIN_CL, LSBFIRST, BUFFER[x + width()]);
	}
	digitalWrite(PIN_LAT, 1);
	digitalWrite(PIN_LAT, 0);
}

void AESYS9929B::dim(uint8_t brightness) {
	analogWrite(PIN_OE, 255 - brightness);
}