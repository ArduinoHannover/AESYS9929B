#ifndef AESYS9929B_H
#define AESYS9929B_H

#include <Adafruit_GFX.h>

class AESYS9929B : public Adafruit_GFX {
	private: 
		const uint8_t
			PIN_DH,
			PIN_CH,
			PIN_DL,
			PIN_CL,
			PIN_LAT,
			PIN_OE;
#ifdef ESP8266
		const uint16_t
			PINB_DH,
			PINB_CH,
			PINB_DL,
			PINB_CL;
#endif //ESP8266
		uint8_t* BUFFER;
	public:
		AESYS9929B(
			uint8_t width,
			uint8_t data_high,
			uint8_t clock_high,
			uint8_t data_low,
			uint8_t clock_low,
			uint8_t latch,
			uint8_t output_enable
		);
		void drawPixel(int16_t x, int16_t y, uint16_t color);
		void fillScreen(uint16_t color);
		void display();
		void dim(uint8_t brightness);
};

#endif //AESYS9929B_H