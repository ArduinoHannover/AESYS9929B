#include <AESYS9929B.h>

#ifdef ESP8266 //Wemos D1 Mini
const uint8_t OE    = 16; //D0
const uint8_t LATCH = 14; //D5
const uint8_t DAT_H = 12; //D6
const uint8_t DAT_L = 13; //D7
const uint8_t CLK_H =  2; //D4
const uint8_t CLK_L = 15; //D8
const uint8_t LIGHT =  0;
#else //Atmega
const uint8_t OE    =  5;
const uint8_t LATCH =  6;
const uint8_t DAT_H =  7;
const uint8_t DAT_L =  8;
const uint8_t CLK_H = 10;
const uint8_t CLK_L =  9;
const uint8_t LIGHT =  3; //A3
#endif //ESP8266

const float mean_over = 20.0;

uint16_t b = 0;
uint8_t d = 0;

// Single Panels have left jumper (above first connector) closed. That means common data path
//AESYS9929B led( 28, DAT_L, CLK_H, DAT_L, CLK_L, LATCH, OE); //Side number display
// Longer Panels have right jumper closed which means separate data paths.
// You may change the short panel to the right jumper if you're having issues with artifacts.
AESYS9929B led( 28, DAT_H, CLK_H, DAT_L, CLK_L, LATCH, OE); //Side number display
//AESYS9929B led(160, DAT_H, CLK_H, DAT_L, CLK_L, LATCH, OE); //Side destination display
//AESYS9929B led(200, DAT_H, CLK_H, DAT_L, CLK_L, LATCH, OE); //Front display

void setup() {
	Serial.begin(115200);

	for (uint8_t y = 0; y < led.height(); y++) {
		for (int16_t x = y & 1 ? led.width() - 1 : 0; x < led.width() && x >= 0; y & 1 ? x-- : x++) {
			led.drawPixel(x, y, 1);
			led.display();
			delay(10);
		}
	}
	for (uint16_t x = 0; x < led.width(); x++) {
		for (int8_t y = x & 1 ? led.height() - 1 : 0; y < led.height() && y >= 0; x & 1 ? y-- : y++) {
			led.drawPixel(x, y, 0);
			led.display();
			delay(10);
		}
	}

	for (uint16_t x = 0; x < led.width(); x += 2) {
		led.drawFastVLine(x, 0, led.height(), 1);
		led.display();
		delay(100);
	}

	for (uint8_t y = 0; y < led.height(); y += 2) {
		led.drawFastHLine(0, y, led.width(), 1);
		led.display();
		delay(100);
	}

	for (int8_t y = led.height() - 1; y > 0; y -= 2) {
		led.drawFastHLine(0, y, led.width(), 1);
		led.display();
		delay(100);
	}

	for (uint8_t i = 255; i > 0; i--) {
		led.dim(i);
		delay(10);
	}

	led.fillScreen(0);
	led.setTextWrap(false);
	led.display();
	led.dim(255);

	uint8_t chars = 255 - 33;
	led.setTextSize(2);
	for (int16_t x = led.width(); x > -chars * 12; x--) {
		led.fillScreen(0);
		led.setCursor(x, 1);
		for (uint8_t c = 33; c != 0; c++) {
			led.write(c);
		}
		led.display();
		delay(30);
	}

	led.fillScreen(0);
	led.setCursor(0, 1);
	led.setTextWrap(true);
	led.print("1234567890");
	led.display();
}

void loop() {
	float blevel;
	for (uint8_t i = 0; i < mean_over; i++) {
		blevel += analogRead(LIGHT) / (float)mean_over;
		delay(1000);
	}
	Serial.println(blevel);
	uint8_t i = d;
	if (blevel > 700) {
		d = 255;
	} else if (blevel > 600) {
		d = 127;
	} else {
		d = 50;
	}
	for (; d != i; d < i ? i-- : i++) {
		led.dim(i);
		delay(10);
	}
}