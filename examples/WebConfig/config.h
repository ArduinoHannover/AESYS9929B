#define USE_WIFI_MANAGER //Use WiFiManger instead of ssid and password
//#define ONLY_AP_MODE    //Do not try to connect to base station instead just open an access point
//#define FIX_IP          //Use fixed IP address

//#define ENABLE_OTA      //Enable OTA feature - for security reasons define a password below
//#define USE_AUTH        //Password protect webinterface (basic auth)

#if !defined(USE_WIFI_MANAGER) && !defined(ONLY_AP_MODE)
	#ifdef FIX_IP
		const IPAddress STA_IP(10, 0, 0, 123);
		const IPAddress STA_GW(10, 0, 0, 1);
		const IPAddress STA_SUB(255, 255, 255, 0);
	#endif
#else //For WiFi Manager or AP Only mode
	const IPAddress AP_IP(10,0,0,1);
	const IPAddress AP_SUB(255,255,255,0);
	const char* AP_PASS = NULL; // Set to NULL if no password needed, otherwise set to minimum 8 chars
#endif //!defined(USE_WIFI_MANAGER) && !defined(ONLY_AP_MODE)

#ifndef USE_WIFI_MANAGER
	const char* STA_SSID = "SSID";
	const char* STA_PASS = "password";
#endif //USE_WIFI_MANAGER

#ifdef ENABLE_OTA
	const char* OTA_PASS = "otaPassword";
#endif //ENABLE_OTA

const char* MDNS_NAME = "AESYSdisplay";

#ifdef USE_AUTH
const char* AUTH_USER = "display";
const char* AUTH_PASS = "password";
#endif //USE_AUTH

const uint8_t  OE     =  16; //D0
const uint8_t  LATCH  =  14; //D5
const uint8_t  DAT_H  =  12; //D6
const uint8_t  DAT_L  =  13; //D7
const uint8_t  CLK_H  =   2; //D4
const uint8_t  CLK_L  =  15; //D8
const uint8_t  LIGHT  =   0;
const uint16_t DWIDTH =  40;