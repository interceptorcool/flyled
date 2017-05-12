
// NeoPixelCylon
// This example will move a Cylon Red Eye back and forth across the 
// the full collection of pixels on the strip.
// 
// This will demonstrate the use of the NeoEase animation ease methods; that provide
// simulated acceleration to the animations.
//
//

#include <Arduino.h>

#include <string.h>

#include <NeoPixelBus.h>
//#include <NeoPixelAnimator.h>
//#include <NeoPixelBrightnessBus.h>

#include "board_pin.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <FS.h>

#ifdef eclipse
#include <Servo/src/Servo.h>
#include </home/icool/Arduino/libraries/ArduinoJson-master/ArduinoJson.h>
#else
#include <Servo.h>
#include <ArduinoJson.h>
#endif

#include "PPMInput.hpp"
#include "model_strip.hpp"
#include "led_io.hpp"
#include "FlyBattaryMonitor.hpp"
#include "PowerOn.hpp"
#include "ESP8266HTTPFileUpload.h"
extern "C" {
	#include "user_interface.h"
}

#include "version_num.h"
#include "build_defs.h"


const char completeVersion[] =
{
    VERSION_MAJOR_INIT,
    '.',
    VERSION_MINOR_INIT,
    '-', 'V', '-',
    BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3,
    '-',
    BUILD_MONTH_CH0, BUILD_MONTH_CH1,
    '-',
    BUILD_DAY_CH0, BUILD_DAY_CH1,
    ' T ',
    BUILD_HOUR_CH0, BUILD_HOUR_CH1,
    ':',
    BUILD_MIN_CH0, BUILD_MIN_CH1,
    ':',
    BUILD_SEC_CH0, BUILD_SEC_CH1,
    '\0'
};


// ============================================================================================================
// ============================================================================================================
// ============================================================================================================

// ====================================
// PPM Input
PPMInput<FLYLED_RCV_CH1_PIN> ppm_input_ch1(50);
PPMInput<FLYLED_RCV_CH2_PIN> ppm_input_ch2(50);

TFlyConf			fc;
FlyColorLeds 		flyLeds(fc);

FlyReceiverInputs	rch(fc);
FlyOutputs			outputs(fc);
FlyBattaryMonitor	batMonitor(fc, flyLeds);
PowerOn				firstStartOnPower(fc);

// ====================================
// ====================================
// WiFi
ESP8266WebServer server(80);
DNSServer dnsServer;
const uint8_t DNS_PORT = 53;
ESP8266HTTPUpdateServer *httpUpdater = nullptr;

ESP8266HTTPFileUpload	*httpUpload = nullptr;

// ожидание подключения по wifi
static const uint16_t c_wifi_blink_time_no_client = 250;
static const uint16_t c_wifi_blink_time_client = 100;
uint16_t wifi_blink_time = c_wifi_blink_time_no_client;

bool Wifi_Enable = false;

// ====================================
WiFiServer telnetServer(23);
WiFiClient telnetClient;

// ====================================
uint32_t time_show;

String showHead()
{
    String head = "";
    head.concat( "\r\n\r\nFLYLED Controller build: ");
    head.concat(completeVersion);
	head.concat( F("\r\n\nHeap: ") );
	head.concat(system_get_free_heap_size());
	head.concat( F("\r\nBoot Vers: ") );
	head.concat(system_get_boot_version());
	head.concat( F("\r\nCPU: ") );
	head.concat(system_get_cpu_freq());
	head.concat( F("\r\nSDK: ") );
	head.concat(system_get_sdk_version());
	head.concat( F("\r\nChip ID: ") );
	head.concat(system_get_chip_id());
	head.concat( F("\r\nFlash ID: "));
	head.concat(spi_flash_get_id());
	head.concat( F("\r\nFlash Size: ") );
	head.concat(ESP.getFlashChipRealSize()/1024); Serial.println( F(" K") );
	head.concat( F("\r\nVcc: ") );
	head.concat(ESP.getVcc());
	head.concat( F("\r\n") );

	return head;
}

void setup()
{
#if 1
    Serial.begin(115200);
    delay(10);

    Serial.print(showHead());

    //delay(500);

    //Serial.setDebugOutput(true);
#endif

    // ==================
    // init ports
    pinMode(FLYLED_STATUS_LED_PIN, OUTPUT);
	pinMode(FLYLED_BUTTON_PIN, INPUT_PULLUP);

    // ==================
    // FFS - init
    //String _config_file_name = "/config.json";

    SPIFFS.begin();

	{
		for (int i = 0; i < 5; ++i)
		{
			digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_ON);
			delay(25);
			digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_OFF);
			delay(50);
		}

    	if (!SPIFFS.exists("/formatComplete.txt"))
    	{
			Serial.println("Please wait 30 secs for SPIFFS to be formatted");
			bool r = SPIFFS.format();

			Serial.printf("\n\nSpiffs formatted: %s\n", r ? "OK":"FAIL"); ;

			File f = SPIFFS.open("/formatComplete.txt", "w");
			if (!f) {
				Serial.println("file open failed");
			} else {
				f.println("Format Complete");
			}
		} else {
			Serial.println("SPIFFS is ready...");
		 }
//	} else {
//		// find config
//		Dir dir = SPIFFS.openDir("/");
//		while (dir.next())
//		{
//			String file = dir.fileName();
//			if (file.indexOf("config") > 0)
//		}
	}

	// ==================
    // config init
    fc.setup();

	// ==================
	// init sys

    flyLeds.setup();

	ppm_input_ch1.setup();
	ppm_input_ch2.setup();

	rch.setup();
	outputs.setup();
	batMonitor.setup();
	firstStartOnPower.setup();

    {
		uint8_t wait_button_press = 5;

		while (!Wifi_Enable && wait_button_press--)
		{
			Wifi_Enable = (digitalRead(FLYLED_BUTTON_PIN) == LOW);
			for (auto i = 0; i < 2; i++)
			{
				digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_ON);
				delay(50);
				digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_OFF);
				delay(50);
			}
			digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_OFF);
			delay(300);
		}
	}

    if (Wifi_Enable)
    {
    	// Do a little work to get a unique-ish name. Append the
		// last two bytes of the MAC (HEX'd) to "Thing-":
		uint8_t mac[WL_MAC_ADDR_LENGTH];
		WiFi.softAPmacAddress(mac);
		String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
					   String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
		macID.toUpperCase();
		String AP_NameString = "FLYLED " + macID;

		uint8_t ap_s = AP_NameString.length() + 1;

		char ap_ssid[ap_s];

		AP_NameString.toCharArray(ap_ssid, ap_s, 0);

		WiFi.mode(WIFI_AP);
		//WiFi.persistent(false);
		WiFi.softAP(ap_ssid);

		//SERVER INIT

		server.on("/", HTTP_GET, [AP_NameString](){

			String message = F ("<h1>");

			message.concat(AP_NameString);
			message.concat(F("</h1>"));
			message.concat( R"(<h2><a href="/upload">UPLOAD JSON Config for FLY LED Controller</a></h2>)" );
			message.concat("<p><br>Current FLYLED Controller build: ");
			message.concat(completeVersion);
			message.concat("</p>");
			message.concat( R"(<p><br><a href="/update">REPLACE SW IN FLY LED Controller</a></p>)" );

			server.send(200,  "text/html", message);
		});

		server.onNotFound([]() {
			String message = "Page not exist!\n\n";
			message += "URI: ";
			message += server.uri();

			server.send(200, "text/plain", message);
		});

		httpUpdater = new ESP8266HTTPUpdateServer();
		httpUpdater->setup(&server);

		httpUpload = new ESP8266HTTPFileUpload();
		httpUpload->setup(&server, "config.json", true);

		server.begin();

		// =============================

		// modify TTL associated  with the domain name (in seconds)
		// default is 60 seconds
		dnsServer.setTTL(300);
		// set which return code will be used for all other domains (e.g. sending
		// ServerFailure instead of NonExistentDomain will reduce number of queries
		// sent by clients)
		// default is DNSReplyCode::NonExistentDomain
		dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);

		// start DNS server for a specific domain name
		{
			IPAddress ip = WiFi.softAPIP();
			dnsServer.start(DNS_PORT, "flyled.local", ip);
		}

		// =============================
		telnetServer.begin();
		telnetServer.setNoDelay(true);

    } else {
		WiFi.disconnect(true);
		WiFi.mode(WIFI_OFF);

		//ppm_input_ch2.setCentrePPM(1530);
    }

    time_show = 0;
}

bool client_connecting = false;

uint32_t wait_for_reboot = 5 * 1000 / c_wifi_blink_time_no_client;

uint32_t time_telnet_step = 0;
uint32_t time_telnet_run;

const char *_save_action;

void loop()
{
    // this is all that is needed to keep it running
    // and avoiding using delay() is always a good thing for
    // any timing related routines

	bool show = false;

    if (Wifi_Enable)
    {
		dnsServer.processNextRequest();
		server.handleClient();

		// look for Client connect trial
		if (telnetServer.hasClient())
		{
			if (!telnetClient || !telnetClient.connected())
			{
				if (telnetClient)
				{
					telnetClient.stop();
					Serial.println("Telnet Client Stop");
				}
				telnetClient = telnetServer.available();
				Serial.println("New Telnet client");
				telnetClient.flush();  // clear input buffer, else you get strange characters
				telnetClient.print(showHead());
				time_telnet_run = millis() + 300000;
			}
		}

   		if (time_telnet_step < (millis()))
		{
   			time_telnet_step = millis() + 1000;

			Serial.print( F("W0 - Heap: ") ); Serial.println(system_get_free_heap_size());

			if (telnetClient && telnetClient.connected())
			{
				String dbg = "";
				dbg.concat( F("Vbat: ") );
				float f = float(analogRead(A0)) / 1023.0 * 13.0;
				dbg.concat( f );
				dbg.concat( F(" CH1: ") );
				dbg.concat(ppm_input_ch1.read_pct()*100);
				dbg.concat( F(" CH2: ") );
				dbg.concat(ppm_input_ch2.read_pct()*100);

				telnetClient.println(dbg);

				while(telnetClient.available())
				{
					if ((char)telnetClient.read() == '\n' ||
						time_telnet_run < millis())
					{
						telnetClient.println("* Closing session by inactivity");
						telnetClient.stopAll();
					}
				}

			}
		}

		// =========================================
        if (time_show < (millis()))
        {
			time_show = millis() + wifi_blink_time;

			digitalWrite(FLYLED_STATUS_LED_PIN, !digitalRead(FLYLED_STATUS_LED_PIN));

			uint8_t softap_stations_cnt = wifi_softap_get_station_num(); //- See more at: http://www.esp8266.com/viewtopic.php?f=32&t=5669#sthash.eKwRDqlw.dpuf

			if (softap_stations_cnt)
			{
				wifi_blink_time = c_wifi_blink_time_client;
				client_connecting = true;
			} else {
				wifi_blink_time = c_wifi_blink_time_no_client;

				if (client_connecting && !wait_for_reboot--)
				{
					digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_OFF);
					delay(1000);
					ESP.restart();
				}
			}
        }
    } else {
    	uint	button_press_time = 0;
    	if (fc.ready())
    	{

#if defined(SET_BUTTON_BRIGHT)
    		// нажатие кнопки на настройку яркости
    		if (digitalRead(FLYLED_BUTTON_PIN) == LOW)
    		{
    			// нужно ждем время
    			if (button_press_time + 3000 > millis())
    			{
    				uint flash_led0 = millis();
    				uint toggle = 0;

    				flyLeds.SetLED0PixelColor(RgbColor(255,255,255));

    				// button free
    				while (digitalRead(FLYLED_BUTTON_PIN) == LOW)
    					yield();

    				// TODO кнопка нажата 3 сек - можно подобрать яркость
    				// берем яркость из конфига
    				uint8_t config_bright = fc.getRoot()[SETMAXIMUMBRIGTH];
    				do {
    					yield();
    					if (ppm_input_ch1.read_pct() > 0.0)
    					{
    						// яркость вверх
    						if (config_bright < 254)
    							config_bright++;
    					} else {
    						// яркость вниз
    						if (config_bright > 2)
    							config_bright--;
    					}
    					flyLeds.SetMAXBrightness(config_bright);
    					if (toggle)
    					{
    						flyLeds.SetLED0PixelColor(RgbColor(255,255,255));
    					} else {
    						flyLeds.SetLED0PixelColor(RgbColor(0,0,0));
    					}
    					toggle = ~toggle;
    					delay(100);
    				} while (digitalRead(FLYLED_BUTTON_PIN) == HIGH);
    			}
    		} else {
    			button_press_time = millis();
    		}

#endif

    		batMonitor.Update();
    		rch.Update();

    		const char *action = nullptr;

			action = firstStartOnPower.getIsolationAction(action);

    		switch(firstStartOnPower.getIsolationStat())
    		{
    		case PowerOn::status::Error:
    		case PowerOn::status::Stop:
    			action = rch.getAction(action);
				break;
    		case PowerOn::status::Send:
    		case PowerOn::status::WaitForTransmitt:
    		default:;
    		}

    		action = flyLeds.StartAction(action, batMonitor.getAction());
			action = outputs.StartAction(action);

    		flyLeds.UpdateAnimations();

    		//if (action == nullptr)
    		flyLeds.Show();
#ifdef DEBUG
    		if (time_show < (millis()))
    		{
    			time_show = millis() + 1000;

       			Serial.print( F("CH1: ") ); Serial.print(ppm_input_ch1.read_pct()*100);
       			Serial.print( F(" = CH2: ") ); Serial.println(ppm_input_ch2.read_pct()*100);

    		}
#endif
    	} else {
			digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_ON);
			delay(50);
			digitalWrite(FLYLED_STATUS_LED_PIN, FLYLED_STATUS_LED_OFF);
			delay(2000);
    	}
    }
}
