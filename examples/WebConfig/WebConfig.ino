#include <AESYS9929B.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>

//#define ONLY_AP_MODE //Do not try to connect to base station instead just open an access point

const uint8_t OE    = 16; //D0
const uint8_t LATCH = 14; //D5
const uint8_t DAT_H = 12; //D6
const uint8_t DAT_L = 13; //D7
const uint8_t CLK_H =  2; //D4
const uint8_t CLK_L = 15; //D8
const uint8_t LIGHT =  0;

/*
 * First Panels have left jumper (above first connector) closed. That means common data path
 */
//AESYS9929B led( 28, DAT_L, CLK_H, DAT_L, CLK_L, LATCH, OE); //Side number display
AESYS9929B led( 40, DAT_L, CLK_H, DAT_L, CLK_L, LATCH, OE); //Single full display
//AESYS9929B led(120, DAT_L, CLK_H, DAT_L, CLK_L, LATCH, OE); //Side destination display
//AESYS9929B led(160, DAT_L, CLK_H, DAT_L, CLK_L, LATCH, OE); //Front display
/*
 * Panels 2+ have right jumper closed which means separate data paths.
 * You may change the first panel to the right jumper if you're having issues with artifacts.
 */
//AESYS9929B led( 28, DAT_H, CLK_H, DAT_L, CLK_L, LATCH, OE); //Side number display

const IPAddress AP_IP(10,0,0,1);
const IPAddress AP_SUB(255,255,255,0);

const char* AP_PASS = NULL; // Set to NULL if no password needed, otherwise set to minimum 8 chars

// You don't need to change anything from here on

#ifndef ONLY_AP_MODE
	#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#endif //ONLY_AP_MODE

ESP8266WebServer server(80);

enum DisplayType {SMALLTEXT, BIGTEXT, MARQUEE};
enum FadeType {
	FADE_APPEAR,
	FADE_DOWN,
	FADE_LEFT,
	FADE_RIGHT,
	FADE_UP,
	FADE_SLIDE_DOWN,
	FADE_SLIDE_LEFT,
	FADE_SLIDE_RIGHT,
	FADE_SLIDE_UP,
	FADE_DIM
};
const char FadeTypeNames[][20] {
	"Appear",
	"Fade down",
	"Fade left",
	"Fade right",
	"Fade up",
	"Slide bottom",
	"Slide left",
	"Slide right",
	"Slide top",
	"Dim"
};
struct DisplayContent {
	boolean active;
	DisplayType display;
	uint16_t speed;
	uint16_t duration;
	FadeType fadeIn;
	FadeType fadeOut;
	uint16_t fadeInSpeed;
	uint16_t fadeOutSpeed;
	char* text;
	DisplayContent* previous;
	DisplayContent* next;
};
DisplayContent* firstNode = NULL;
DisplayContent* lastNode = NULL;
DisplayContent* currentNode = NULL;

uint8_t maxBrightness = 255;
boolean cycleOn = true;
uint32_t
	animationStart = 0,
	displayStart   = 0,
	animationEnd   = 0;

void setup() {
	Serial.begin(115200);
	led.dim(255);
	
	char buf[20];
	strcpy(buf, "AESYS_");
	sprintf(&buf[strlen(buf)], "%x", ESP.getChipId());
#ifdef ONLY_AP_MODE
	WiFi.mode(WIFI_AP);
	WiFi.softAPConfig(AP_IP, AP_IP, AP_SUB);
	WiFi.softAP(buf, AP_PASS);
	marquee(String("Connected to ") + buf + " and open " + AP_IP.toString(), 20);
#else
	Serial.println("Autoconnect");
	marquee("Connecting...", 20);
	WiFiManager wifiManager;
	wifiManager.setAPStaticIPConfig(AP_IP, AP_IP, AP_SUB);
	wifiManager.setAPCallback(cannotConnect);
	wifiManager.autoConnect(buf);
	wifiManager.autoConnect(buf, AP_PASS);
	marquee(String("Connected: ")+WiFi.localIP().toString(), 20);
#endif //ONLY_AP_MODE
	
	server.on("/", showIndex);
	server.on("/config.csv", showConfig);
	server.begin();
	SPIFFS.begin();
	parseConfig();
}

void marquee(String text, uint16_t speed) {
	led.setTextWrap(false);
	led.setTextSize(2);
	int32_t l = text.length();
	l *= -12;
	for (int32_t x = led.width(); x > l; x--) {
		uint32_t nextchar = millis() + speed;
		led.fillScreen(0);
		led.setCursor(x, 1);
		led.print(text);
		led.display();
		while (nextchar > millis()) yield();
		yield();
	}
}

void cannotConnect(WiFiManager *wifiManager) {
	Serial.println("Cannot connect");
	String text = "Cannot connect to last known WiFi. Please connect to ";
	text += wifiManager->getConfigPortalSSID();
	text += " and open ";
	text += AP_IP.toString();
	marquee(text, 20);
}

void parseConfig() {
	Serial.println("(Re)loading data");
	while (deleteNode(currentNode));
	{	
		DisplayContent* node = (DisplayContent*)calloc(1, sizeof(DisplayContent));
		node->active = 1;
		node->display = MARQUEE;
		node->speed = 20;
		node->duration = 10000;
		node->fadeIn = FADE_APPEAR;
		node->fadeOut = FADE_APPEAR;
		node->fadeInSpeed = 10;
		node->fadeOutSpeed = 10;
		insertNodeBeforeLast(node);
	}
	File f = SPIFFS.open("/config.csv", "r");
	if (f) {
		while (f.available()) {
			String cont = f.readStringUntil('\n');
			Serial.println(cont);
			DisplayContent* node = (DisplayContent*)calloc(1, sizeof(DisplayContent));
			uint8_t pos = 0;
			uint8_t npos = cont.indexOf(';', pos);
			node->active = atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->display = (DisplayType)atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->speed = atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->duration = atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->fadeIn = (FadeType)atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->fadeOut = (FadeType)atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->fadeInSpeed = atoi(cont.substring(pos, npos).c_str());
			pos = npos + 1;
			npos = cont.indexOf(';', pos);
			node->fadeOutSpeed = atoi(cont.substring(pos, npos).c_str());
			String t = cont.substring(npos + 1);
			node->text = (char*)calloc(t.length() + 1, sizeof(char));
			strcpy(node->text, t.c_str());
			insertNodeBeforeLast(node);
			Serial.println("Node added");
		}
		f.close();
	}
	currentNode = firstNode;
	if (cycleOn) {
		while (!currentNode->active && currentNode != lastNode) {
			currentNode = currentNode->next;
		}
		animationStart = millis();
		displayStart = 0;
		animationEnd = 0;
	}
}

void saveConfig() {
	DisplayContent* dc = firstNode;
	File f = SPIFFS.open("/config.csv", "w");
	while (dc->text != NULL) {
		f.write(dc->active ? '1' : '0');
		f.write(';');
		f.print(dc->display, DEC);
		f.write(';');
		f.print(dc->speed, DEC);
		f.write(';');
		f.print(dc->duration, DEC);
		f.write(';');
		f.print(dc->fadeIn, DEC);
		f.write(';');
		f.print(dc->fadeOut, DEC);
		f.write(';');
		f.print(dc->fadeInSpeed, DEC);
		f.write(';');
		f.print(dc->fadeOutSpeed, DEC);
		f.write(';');
		String t = dc->text;
		t.replace("\n", "\\n");
		f.print(t);
		f.write('\n');
		dc = dc->next;
	}
	f.close();
}

void showConfig() {
	File f = SPIFFS.open("/config.csv", "r");
	if (f) {
		server.streamFile(f, "text/csv");
		f.close();
	}
}

DisplayContent* getNode(uint8_t index) {
	uint8_t i = 0;
	if (firstNode == NULL) return NULL;
	DisplayContent* dc = firstNode;
	while (i < index && dc->text != NULL) {
		dc = dc->next;
		if (dc == NULL) return NULL;
		i++;
	}
	if (i < index) {
		return NULL;
	}
	return dc;
}

bool deleteNode(uint8_t index) {
	DisplayContent* dc = getNode(index);
	return deleteNode(dc);
}

bool deleteNode(DisplayContent* dc) {
	if (dc == NULL) return false;
	if (dc->previous != NULL) {
		(dc->previous)->next = dc->next;
	} else {
		firstNode = dc->next; // NULL if only element
	}
	if (dc->next != NULL) {
		(dc->next)->previous = dc->previous;
	} else {
		lastNode = dc->previous; // NULL if only element
	}
	free(dc);
	return true;
}

void insertNodeBeforeLast(DisplayContent* dc) {
	if (dc == NULL) return;
	dc->next = lastNode; //Empty element or NULL if first inserted
	if (lastNode != NULL) {
		if (lastNode->previous != NULL) {
			dc->previous = lastNode->previous;
			(dc->previous)->next = dc;
		} else {
			firstNode = dc;
		}
		lastNode->previous = dc;
	} else {
		lastNode = dc;
	}
	if (firstNode == NULL) {
		firstNode = dc;
	}
}

void getText(String& text) {
	text = currentNode->text;
	text.replace("\xC3\x84", "\x8E"); //Ä
	text.replace("\xC3\xA4", "\x84"); //ä
	text.replace("\xC3\x96", "\x99"); //Ö
	text.replace("\xC3\xB6", "\x94"); //ö
	text.replace("\xC3\x9C", "\x9A"); //Ü
	text.replace("\xC3\xBC", "\x81"); //ü
	text.replace("\xC3\x9F", "\xE0"); //ß
}

void showIndex() {
	if (server.method() == HTTP_POST) {
		if (server.hasArg("brightness")) {
			maxBrightness = min(255, max(10, atoi(server.arg("brightness").c_str())));
		}
		if (server.hasArg("setCycle")) {
			cycleOn = server.hasArg("cycle");
		}
		if (server.hasArg("add") | server.hasArg("edit")) {
			if (
				!server.hasArg("displaytype") |
				!server.hasArg("speed") |
				!server.hasArg("duration") |
				!server.hasArg("fadeIn") |
				!server.hasArg("fadeOut") |
				!server.hasArg("fadeInSpeed") |
				!server.hasArg("fadeOutSpeed") |
				!server.hasArg("text")
				) {
				server.send(400, "text/plain", "Missing param.");
			}
			DisplayContent* node;
			if (server.hasArg("add")) {
				Serial.println("add");
				node = (DisplayContent*)calloc(1, sizeof(DisplayContent));
			} else {
				Serial.println("edit");
				node = getNode(atoi(server.arg("id").c_str()));
				if (node == NULL) {
					server.send(400, "text/plain", "ID does not exist.");
					return;
				}
			}
			
			node->active = server.hasArg("active");
			node->display = (DisplayType)atoi(server.arg("displaytype").c_str());
			node->speed = max(1, atoi(server.arg("speed").c_str()));
			node->duration = atoi(server.arg("duration").c_str());
			node->fadeIn = (FadeType)atoi(server.arg("fadeIn").c_str());
			node->fadeOut = (FadeType)atoi(server.arg("fadeOut").c_str());
			node->fadeInSpeed = max(1, atoi(server.arg("fadeInSpeed").c_str()));
			node->fadeOutSpeed = max(1, atoi(server.arg("fadeOutSpeed").c_str()));
			node->text = (char*)realloc(node->text, sizeof(char) * (server.arg("text").length() + 1));
			strcpy(node->text, server.arg("text").c_str());
			
			if (server.hasArg("add")) {
				insertNodeBeforeLast(node);
			}
			saveConfig();
		}
		if (server.hasArg("delete")) {
			if (!deleteNode(atoi(server.arg("id").c_str()))) {
				server.send(400, "text/plain", "ID does not exist.");
			}
			saveConfig();
		}
		if (server.hasArg("show")) {
			currentNode = getNode(atoi(server.arg("id").c_str()));
			animationStart = millis();
			displayStart = 0;
			animationEnd = 0;
			Serial.println("show");
		}
	}

	server.sendContent("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
	
	server.sendContent("<html>\n<head>\n<title>AESYSconf</title>\n<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\" />\n<style>input[type=number]{width:100px;}\n.table{display:table}\nform,.headline{display:table-row}\n.cell{display:table-cell}\n.desc{display:none}\n.head{display:table-cell}\n@media screen and (max-width:1000px){\n\tinput[type=submit]{width:33%;}\n\t.table{display:block}\n\tform{display:block;}\n\t.headline{display:none;}\n\t.desc{display:inline-block;width:50%;border-bottom:thin solid black}\n\t.cell{display:inline-block;}\n\t.cell:first-child,.cell:last-child{display:block;}\n}\n</style>\n<head>\n<body>\n<h2>AESYSconf</h2>");
	DisplayContent* dc = firstNode;
	server.sendContent(String("Max width for small text (two lines each): ") + (led.width() / 6) + "\n<br>\nMax width for big text (single line): " + (led.width() / 12) + "\n<br>\n");
	server.sendContent(String("<form method=\"post\">\nBrightness\n<input type=\"number\" name=\"brightness\" min=\"10\" max=\"255\" value=\"") + maxBrightness + "\" />\n<input type=\"submit\" value=\"Set\" />\n</form>\n<br>\n");
	server.sendContent(String("<form method=\"post\">\nCycle (off=static)\n<input type=\"checkbox\" name=\"cycle\"") + (cycleOn ? " checked" : "") + " />\n<input type=\"submit\" name=\"setCycle\" value=\"Set\" />\n</form>\n<br>\n");
	server.sendContent("<div class=\"table\">\n<div class=\"headline\">\n<div class=\"head\">ID</div>\n<div class=\"head\">Active</div>\n<div class=\"head\">Display</div>\n<div class=\"head\">Text</div>\n<div class=\"head\">Marquee Speed</div>\n<div class=\"head\">Duration (ms)</div>\n<div class=\"head\">Fade In</div>\n<div class=\"head\">Speed</div>\n<div class=\"head\">Fade Out</div>\n<div class=\"head\">Speed</div>\n<div class=\"head\">Action</div>\n</div>");
	uint8_t i = 0;
	while (dc != NULL) {
		server.sendContent("<form method=\"post\">\n<div class=\"cell\">\n");
		if (dc->text == NULL) {
			server.sendContent("New");
		} else {
			server.sendContent(String(i));
		}
		server.sendContent(String("</div>\n<div class=\"desc\">Active</div>\n<div class=\"cell\">\n<input name=\"active\" type=\"checkbox\"") + (dc->active ? " checked" : "") + " />\n</div>\n<div class=\"desc\">Type</div>\n");
		server.sendContent("<div class=\"cell\">\n<select name=\"displaytype\">\n");
		server.sendContent(String("<option value=\"0\"") + (dc->display == SMALLTEXT ? " selected" : "") + ">Small text (2 lines)</option>\n");
		server.sendContent(String("<option value=\"1\"") + (dc->display == BIGTEXT ? " selected" : "") + ">Big text (single line)</option>\n");
		server.sendContent(String("<option value=\"2\"") + (dc->display == MARQUEE ? " selected" : "") + ">Marquee (single line)</option>\n");
		server.sendContent(String("</select>\n</div>\n<div class=\"desc\">\nText\n</div>\n<div class=\"cell\">\n<input style=\"font-family:monospace\" name=\"text\" type=\"text\" style=\"width: 100%;\" value=\"") + (dc->text == NULL ? "" : dc->text) + "\" />\n</div>\n");
		server.sendContent(String("<div class=\"desc\">\nMarquee speed\n</div>\n<div class=\"cell\">\n<input type=\"number\" min=\"1\" name=\"speed\" value=\"") + dc->speed + "\" />\n</div>\n");
		server.sendContent(String("<div class=\"desc\">\nDisplay duration\n</div>\n<div class=\"cell\">\n<input type=\"number\" min=\"0\" name=\"duration\" value=\"") + dc->duration + "\" />\n</div>\n");
		for (uint8_t j = 0; j < 2; j++) {
			FadeType f = j == 0 ? dc->fadeIn : dc->fadeOut;
			server.sendContent("<div class=\"desc\">\n");
			server.sendContent(j == 0 ? "Fade In</div>\n<div class=\"cell\">\n<select name=\"fadeIn\">" : "Fade Out</div>\n<div class=\"cell\">\n<select name=\"fadeOut\">\n");
			for (uint8_t i = 0; i < sizeof(FadeTypeNames) / sizeof(FadeTypeNames[0]) / sizeof(FadeTypeNames[0][0]); i++) {
				server.sendContent(String("<option value=\"") + i + "\"" + (f == i ? " selected" : "") + ">" + FadeTypeNames[i] + "</option>\n");
			}
			server.sendContent(String("</select>\n</div>\n<div class=\"desc\">Fade Speed</div>\n<div class=\"cell\"><input type=\"number\" min=\"1\" name=\"") + (j == 0 ? "fadeInSpeed" : "fadeOutSpeed") + "\" value=\"" + (j == 0 ? dc-> fadeInSpeed : dc->fadeOutSpeed) + "\" /></div>\n");
		}
		if (dc->text == NULL) {
			server.sendContent("<div class=\"cell\"><input type=\"submit\" name=\"add\" value=\"Add\" /></div>\n");
		} else {
			server.sendContent(String("<div class=\"cell\"><input type=\"hidden\" name=\"id\" value=\"") + i + "\" />\n");
			server.sendContent("<input type=\"submit\" name=\"edit\" value=\"Update\" />\n<input type=\"submit\" name=\"delete\" value=\"Delete\" />\n<input type=\"submit\" name=\"show\" value=\"Show\" /></div>\n");
		}
		server.sendContent("</form>");
		i++;
		if (dc->next == NULL) break;
		dc = dc->next;
	}
	server.sendContent("</div>\n</body>\n</html>");
}

bool animate() {
	bool end = animationEnd;
	FadeType f = end ? currentNode->fadeOut : currentNode->fadeIn;
	int16_t frame = (millis() - (end ? animationEnd : animationStart)) / (end ? currentNode->fadeOutSpeed : currentNode->fadeInSpeed);
	if (currentNode->display == BIGTEXT) {
		led.setTextSize(2);
	} else {
		led.setTextSize(1);
	}
	led.fillScreen(0);
	String text;
	getText(text);
	if (f == FADE_SLIDE_LEFT || f == FADE_SLIDE_RIGHT || f == FADE_SLIDE_UP || f == FADE_SLIDE_DOWN) {
		led.setTextWrap(false);
		int16_t x = 0;
		int16_t y = currentNode->display == BIGTEXT;
		switch (f) {
			case FADE_SLIDE_LEFT:
				x = end ? -frame : (-led.width() + frame);
				break;
			case FADE_SLIDE_RIGHT:
				x = end ? frame : (led.width() - frame);
				break;
			case FADE_SLIDE_UP:
				y += end ? -frame : (-led.height() + frame);
				break;
			case FADE_SLIDE_DOWN:
				y += end ? frame : (led.height() - frame);
				break;
			default:
				break;
		}
		led.setCursor(x, y);
		if (currentNode->display == BIGTEXT) {
			led.print(text.substring(0, led.width() - 1 / 12));
		}
		if (currentNode->display == SMALLTEXT) {
			led.print(text.substring(0, led.width() / 6));
			led.setCursor(x, y + 8);
			led.print(text.substring(led.width() / 6, (led.width() / 6) * 2));
		}
		led.display();
		led.dim(maxBrightness);
		if (f == FADE_SLIDE_LEFT || f == FADE_SLIDE_RIGHT) {
			return frame >= led.width();
		}
		return frame >= led.height();
	}
	led.setTextWrap(true);
	led.setCursor(0, currentNode->display == BIGTEXT);
	led.print(text);
	switch (f) {
		case FADE_DIM:
			led.dim(min((uint16_t)maxBrightness, (uint16_t)(end ? maxBrightness - frame : frame)));
			led.display();
			return frame >= maxBrightness;
		case FADE_LEFT:
			for (uint16_t i = 0; i < (end ? frame : led.width() - frame); i++) {
				led.drawFastVLine(i, 0, led.height(), 0);
			}
			led.display();
			led.dim(maxBrightness);
			return frame > led.width();
		case FADE_RIGHT:
			for (uint16_t i = 0; i < (end ? frame : led.width() - frame); i++) {
				led.drawFastVLine(led.width() - i - 1, 0, led.height(), 0);
			}
			led.display();
			led.dim(maxBrightness);
			return frame > led.width();
		case FADE_DOWN:
			for (uint16_t i = 0; i < (end ? frame : led.height() - frame); i++) {
				led.drawFastHLine(0, i, led.width(), 0);
			}
			led.display();
			led.dim(maxBrightness);
			return frame > led.height();
		case FADE_UP:
			for (uint16_t i = 0; i < (end ? frame : led.height() - frame); i++) {
				led.drawFastHLine(0, led.height() - i - 1, led.width(), 0);
			}
			led.display();
			led.dim(maxBrightness);
			return frame > led.height();
		default:
			return true;
	}
}

void loop() {
	server.handleClient();
	if (currentNode != NULL && currentNode->text != NULL) {
		if (animationStart != 0 && displayStart == 0) {
			bool next = (currentNode->fadeIn == FADE_APPEAR) | (currentNode->display == MARQUEE);
			if (!next) {
				next = animate();
			}
			if (next) {
				displayStart = millis();
				animationEnd = 0;
			}
		}
		if (displayStart != 0 && animationEnd == 0) {
			bool fout = false;
			led.dim(maxBrightness);
			String text;
			getText(text);
			if (currentNode->display == MARQUEE) {
				led.setTextWrap(false);
				led.setTextSize(2);
				int32_t l = text.length();
				l *= -12;
				int32_t x = led.width() - (millis() - displayStart) / currentNode->speed;
				if (x < l) {
					fout = true;
				}
				led.fillScreen(0);
				led.setCursor(x, 1);
				led.print(text);
				led.display();
			} else {
				led.setTextWrap(true);
				if (currentNode->display == BIGTEXT) {
					led.setTextSize(2);
					led.setCursor(0, 1);
				} else {
					led.setTextSize(1);
					led.setCursor(0, 0);
				}
				led.fillScreen(0);
				led.print(text);
				led.display();
				if (displayStart + currentNode->duration < millis() && cycleOn) {
					fout = true;
				}
			}
			if (fout) {
				animationEnd = millis();
			}
		}
		if (displayStart != 0 && animationEnd != 0) {
			bool next = (currentNode->fadeIn == FADE_APPEAR) | (currentNode->display == MARQUEE);
			if (!next) {
				next = animate();
			}
			if (next) {
				animationStart = millis();
				displayStart = 0;
				animationEnd = 0;
				if (cycleOn) {
					DisplayContent* dc = currentNode;
					do {
						if (currentNode->next == lastNode) {
							currentNode = firstNode;
						} else {
							currentNode = currentNode->next;
						}
					} while (!currentNode->active && currentNode != dc);
				}
			}
		}
	}
}