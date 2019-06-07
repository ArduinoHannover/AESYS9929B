#include <AESYS9929B.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <FS.h>

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

IPAddress AP_IP(10,0,0,1);
IPAddress AP_Sub(255,255,255,0);

ESP8266WebServer server(80);

enum DisplayType {SMALLTEXT, BIGTEXT, MARQUEE};
enum FadeType {FADE_APPEAR, FADE_DOWN, FADE_LEFT, FADE_RIGHT, FADE_UP, FADE_DIM};
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
DisplayContent* content = NULL;
DisplayContent* last = NULL;
DisplayContent* current = NULL;

uint8_t maxBrightness = 255;
boolean cycleOn = true;
uint32_t
	animationStart = 0,
	displayStart   = 0,
	animationEnd   = 0;

void setup() {
	Serial.begin(115200);
	WiFiManager wifiManager;
	led.dim(255);
	wifiManager.setAPStaticIPConfig(AP_IP, AP_IP, AP_Sub);
	
	wifiManager.setAPCallback(cannotConnect);
	
	Serial.println("Autoconnect");

	marquee("Connecting...", 20);
	
	char buf[20];
	strcpy(buf, "AESYS_");
	sprintf(&buf[strlen(buf)], "%x", ESP.getChipId());
	wifiManager.autoConnect(buf);

	marquee(String("Connected: ")+WiFi.localIP().toString(), 20);
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
	Serial.println("Resetting data");
	content = NULL;
	while (last != NULL) {
		DisplayContent* pre = last;
		if (last->previous) {
			pre = last->previous;
			free(last);
			if (pre != NULL) {
				pre->next = NULL;
			}
			last = pre;
		}
	}
	File f = SPIFFS.open("/config.csv", "r");
	if (f) {
		while (f.available()) {
			String cont = f.readStringUntil('\n');
			Serial.println(cont);
			DisplayContent* node = (DisplayContent*)malloc(sizeof(DisplayContent));
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
			node->text = (char*)malloc(sizeof(char) * (t.length() + 1));
			node->previous = NULL;
			node->next = NULL;
			strcpy(node->text, t.c_str());
			if (last == NULL) {
				content = node;
			} else {
				last->next = node;
				node->previous = last;
			}
			last = node;
			Serial.println("Node added");
		}
		f.close();
	}
	DisplayContent* node = (DisplayContent*)malloc(sizeof(DisplayContent));
	node->active = 1;
	node->display = MARQUEE;
	node->speed = 20;
	node->duration = 10000;
	node->fadeIn = FADE_APPEAR;
	node->fadeOut = FADE_APPEAR;
	node->fadeInSpeed = 10;
	node->fadeOutSpeed = 10;
	node->text = NULL;
	node->previous = NULL;
	node->next = NULL;
	if (last == NULL) {
		content = node;
	} else {
		last->next = node;
		node->previous = last;
	}
	last = node;
	current = content;
	if (cycleOn) {
		while (!current->active && current != last) {
			current = current->next;
		}
		animationStart = millis();
		displayStart = 0;
		animationEnd = 0;
	}
}

void saveConfig() {
	DisplayContent* dc = content;
	File f = SPIFFS.open("/config.csv", "w");
	while (dc->text != NULL) {
		Serial.println("Adding node");
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
		Serial.println(t);
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

void showIndex() {
	if (server.method() == HTTP_POST) {
		Serial.println("Post");
		DisplayContent* node;
		if (server.hasArg("id")) {
			Serial.print("Searching for ID ");
			uint8_t i = 0;
			uint8_t d = atoi(server.arg("id").c_str());
			Serial.println(d);
			DisplayContent* dc = content;
			while (i < d && dc->text != NULL) {
				dc = dc->next;
				i++;
			}
			if (i < d) {
				server.send(400, "text/plain", "ID does not exist.");
				return;
			}
			node = dc;
			Serial.println(node->text);
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
			if (server.hasArg("add")) {
				Serial.println("add");
				node = (DisplayContent*)malloc(sizeof(DisplayContent));
			}
			if (server.hasArg("edit")) {
				Serial.println("edit");
			}
			
			node->active = server.hasArg("active");
			node->display = (DisplayType)atoi(server.arg("displaytype").c_str());
			node->speed = max(1, atoi(server.arg("speed").c_str()));
			node->duration = atoi(server.arg("duration").c_str());
			node->fadeIn = (FadeType)atoi(server.arg("fadeIn").c_str());
			node->fadeOut = (FadeType)atoi(server.arg("fadeOut").c_str());
			node->fadeInSpeed = max(1, atoi(server.arg("fadeInSpeed").c_str()));
			node->fadeOutSpeed = max(1, atoi(server.arg("fadeOutSpeed").c_str()));
			if (server.hasArg("add")) {
				node->text = (char*)malloc(sizeof(char) * (server.arg("text").length() + 1));
			} else {
				node->text = (char*)realloc(node->text, sizeof(char) * (server.arg("text").length() + 1));
			}
			strcpy(node->text, server.arg("text").c_str());
			if (server.hasArg("add")) {
				Serial.println("Insert element");
				node->next = last; //Empty element
				node->previous = NULL;
				if (last->previous == NULL) {
					content = node; 
				} else {
					node->previous = last->previous;
					(node->previous)->next = node;
				}
				last->previous = node;
			}
			saveConfig();
		}
		if (server.hasArg("delete")) {
			if (node->previous != NULL) {
				(node->previous)->next = node->next;
			}
			if (node->next != NULL) {
				(node->next)->previous = node->previous;
			}
			if (current == node) {
				current = NULL;
			}
			free(node);
			saveConfig();
		}
		if (server.hasArg("show")) {
			animationStart = millis();
			displayStart = 0;
			animationEnd = 0;
			current = node;
			Serial.println("show");
		}
	}
	
	String html = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><style>.table{display:table}form,.headline{display:table-row}.cell{display:table-cell}.desc{display:none}.head{display:table-cell}</style><head><body><h2>AESYSconf</h2>";
	uint8_t i = 0;
	DisplayContent* dc = content;
	html += "Max width for small text (two lines each): ";
	html += led.width() / 6;
	html += "<br>Max width for big text (single line): ";
	html += led.width() / 12;
	html += "<div class=\"table\"><div class=\"headline\"><div class=\"head\">ID</div><div class=\"head\">Active</div><div class=\"head\">Display</div><div class=\"head\">Text</div><div class=\"head\">Marquee Speed</div><div class=\"head\">Duration (ms)</div><div class=\"head\">Fade In</div><div class=\"head\">Speed</div><div class=\"head\">Fade Out</div><div class=\"head\">Speed</div><div class=\"head\">Action</div></div>";
	while (dc != NULL) {
		Serial.println("Next node");
		html += "<form method=\"post\"><div class=\"cell\">";
		if (dc->text == NULL) {
			html += "Add new";
		} else {
			html += i;
		}
		html += String("</div><div class=\"desc\">Active</div><div class=\"cell\"><input name=\"active\" type=\"checkbox\"") + (dc->active ? " checked" : "") + " /></div><div class=\"desc\">Type</div>";
		html += "<div class=\"cell\"><select name=\"displaytype\">";
		html += String("<option value=\"0\"") + (dc->display == SMALLTEXT ? " selected" : "") + ">Small text (2 lines)</option>";
		html += String("<option value=\"1\"") + (dc->display == BIGTEXT ? " selected" : "") + ">Big text (single line)</option>";
		html += String("<option value=\"2\"") + (dc->display == MARQUEE ? " selected" : "") + ">Marquee (single line)</option>";
		html += String("</select></div><div class=\"desc\">Text</div><div class=\"cell\"><input style=\"font-family:monospace\" name=\"text\" type=\"text\" style=\"width: 100%;\" value=\"") + (dc->text == NULL ? "" : dc->text) + "\" /></div>";
		html += String("<div class=\"desc\">Marquee speed</div><div class=\"cell\"><input type=\"number\" min=\"1\" name=\"speed\" value=\"") + dc->speed + "\" /></div>";
		html += String("<div class=\"desc\">Display duration</div><div class=\"cell\"><input type=\"number\" min=\"0\" name=\"duration\" value=\"") + dc->duration + "\" /></div>";
		for (uint8_t j = 0; j < 2; j++) {
			FadeType f = j == 0 ? f = dc->fadeIn : f = dc->fadeOut;
			html += "<div class=\"desc\">";
			html += j == 0 ? "Fade In</div><div class=\"cell\"><select name=\"fadeIn\">" : "Fade Out</div><div class=\"cell\"><select name=\"fadeOut\">";
			html += String("<option value=\"0\"") + (f == FADE_APPEAR ? " selected" : "") + ">Appear</option>";
			html += String("<option value=\"1\"") + (f == FADE_DOWN ? " selected" : "") + ">Fade down</option>";
			html += String("<option value=\"2\"") + (f == FADE_LEFT ? " selected" : "") + ">Fade left</option>";
			html += String("<option value=\"3\"") + (f == FADE_RIGHT ? " selected" : "") + ">Fade right</option>";
			html += String("<option value=\"4\"") + (f == FADE_UP ? " selected" : "") + ">Fade up</option>";
			html += String("<option value=\"5\"") + (f == FADE_DIM ? " selected" : "") + ">Dim</option>";
			html += "</select></div>";
			html += String("<div class=\"desc\">Fade Speed</div><div class=\"cell\"><input type=\"number\" min=\"1\" name=\"") + (j == 0 ? "fadeInSpeed" : "fadeOutSpeed") + "\" value=\"" + (j == 0 ? dc-> fadeInSpeed : dc->fadeOutSpeed) + "\" /></div>";
		}
		if (dc->text == NULL) {
			html += String("<div class=\"cell\"><input type=\"submit\" name=\"add\" value=\"Save\" /></div>");
		} else {
			html += String("<div class=\"cell\"><input type=\"hidden\" name=\"id\" value=\"") + i + "\" />";
			html += String("<input type=\"submit\" name=\"edit\" value=\"Update\" /><input type=\"submit\" name=\"delete\" value=\"Delete\" /><input type=\"submit\" name=\"show\" value=\"Show\" /></div>");
		}
		html += "</form>";
		i++;
		if (dc->next == NULL) break;
		dc = dc->next;
	}
	html += "</div></body></html>";
	server.send(200, "text/html", html);
}

bool animate() {
	bool end = animationEnd;
	FadeType f = end ? current->fadeOut : current->fadeIn;
	uint16_t frame = (millis() - (end ? animationEnd : animationStart)) / (end ? current->fadeOutSpeed : current->fadeInSpeed);
	led.setTextWrap(true);
	if (current->display == BIGTEXT) {
		led.setTextSize(2);
	} else {
		led.setTextSize(1);
	}
	led.fillScreen(0);
	led.setCursor(0, 1);
	led.print(current->text);
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
	if (current != NULL && current->text != NULL) {
		if (animationStart != 0 && displayStart == 0) {
			bool next = current->fadeIn == FADE_APPEAR | current->display == MARQUEE;
			if (!next) {
				next = animate();
			}
			if (next) {
				displayStart = millis();
				animationEnd = 0;
				Serial.println("Switching to display");
			}
		}
		if (displayStart != 0 && animationEnd == 0) {
			bool fout = false;
			led.dim(maxBrightness);
			if (current->display == MARQUEE) {
				led.setTextWrap(false);
				led.setTextSize(2);
				int32_t l = strlen(current->text);
				l *= -12;
				int32_t x = led.width() - (millis() - displayStart) / current->speed;
				if (x < l) {
					fout = true;
				}
				led.fillScreen(0);
				led.setCursor(x, 1);
				led.print(current->text);
				led.display();
			} else {
				led.setTextWrap(true);
				if (current->display == BIGTEXT) {
					led.setTextSize(2);
				} else {
					led.setTextSize(1);
				}
				led.fillScreen(0);
				led.setCursor(0, 1);
				led.print(current->text);
				led.display();
				if (displayStart + current->duration < millis() && cycleOn) {
					fout = true;
				}
			}
			if (fout) {
				animationEnd = millis();
				Serial.println("Switching to Fadeout");
			}
		}
		if (displayStart != 0 && animationEnd != 0) {
			bool next = current->fadeIn == FADE_APPEAR | current->display == MARQUEE;
			if (!next) {
				next = animate();
			}
			if (next) {
				animationStart = millis();
				displayStart = 0;
				animationEnd = 0;
				if (cycleOn) {
					DisplayContent* dc = current;
					do {
						if (current->next == last) {
							current = content;
						} else {
							current = current->next;
						}
					} while (!current->active && current != dc);
				}
				Serial.println("Switching to Fadein");
			}
		}
	}
}