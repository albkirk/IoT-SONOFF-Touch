/*
 * * ESP8266 template with config web page
 * based on BVB_WebConfig_OTA_V7 from Andreas Spiess https://github.com/SensorsIot/Internet-of-Things-with-ESP8266
 * to do: create button on web page to setup AP or Client WiFi Mode
 */
#include <ESP8266WiFi.h>

#define SWVer 3                           // Software version (use byte 1-255 range values!)
#define ChipID String(ESP.getChipId())    // ChipcID is taken internally
//#define LED_esp 2                         // ESP Led in connected to GPIO 2
#define GPIO4 4                           // GPIO 4 for future usage
#define GPIO5 5                           // GPIO 5 for future usage
#define BUZZER 0                          // (Active) Buzzer

//GPIO SONOFF Touch
#define LED_esp   13                      // in fact it's the WiFi LED
#define RELAY  12                         //Also operates Touch LED
#define TouchInput 0


// **** Normal code definition here ...
#define interval 50                     // time interval window to ignore bouncing
int NOW = 0;                            // timer var to store the current millis() value
int last_touched = 0;                   // timer var to avoid function call trigger due contact bounce
int COUNT_touched = 0;                  // to count number of times TouchInput button was pressed within interval
boolean BTN_touched = false;            // TouchInput button was pressed (true = yes, false = no)
boolean DOUBLE_touched = false;         // TouchInput button was pressed 2 times within interval (true = yes, false = no)
boolean TRIPLE_touched = false;         // TouchInput button was pressed 3 times within interval (true = yes, false = no)
boolean RELAY_STATUS = false;           // status of RELAY (true = Lamp ON, false = Lamp OFF)
boolean Last_STATUS = false;            // latest status of RELAY (true = Lamp ON, false = Lamp OFF)


struct strConfig {
  String DeviceName;                      // 16 Byte - EEPROM 16
  byte  ONTime;                           //  1 Byte - EEPROM 37
  byte  SLEEPTime;                        //  1 Byte - EEPROM 38
  boolean DEEPSLEEP;                      //  1 Byte - EEPROM 39
  boolean LED;                            //  1 Byte - EEPROM 40
  boolean TELNET;                         //  1 Byte - EEPROM 41
  boolean OTA;                            //  1 Byte - EEPROM 42
  boolean WEB;                            //  1 Byte - EEPROM 43
  String ssid;                            // 16 Byte - EEPROM 44
  String WiFiKey;                         // 16 Byte - EEPROM 60
  boolean STAMode;                        //  1 Byte - EEPROM 76
  boolean dhcp;                           //  1 Byte - EEPROM 77
  byte  IP[4];                            //  4 Byte - EEPROM 80
  byte  Netmask[4];                       //  4 Byte - EEPROM 84
  byte  Gateway[4];                       //  4 Byte - EEPROM 88
  String NTPServerName;                   // 32 Byte - EEPROM 128
  long TimeZone;                          //  4 Byte - EEPROM 160
  long Update_Time_Via_NTP_Every;         //  4 Byte - EEPROM 164
  boolean isDayLightSaving;               //  1 Byte - EEPROM 168
  String MQTT_Server;                     // 32 Byte - EEPROM 176
  long MQTT_Port;                         //  4 Byte - EEPROM 208
  String MQTT_User;                       // 16 Byte - EEPROM 212
  String MQTT_Password;                   // 16 Byte - EEPROM 228
  String MQTT_ClientID;                   //  8 Byte - EEPROM 244
  String Location;                        // 16 Byte - EEPROM 252


  // Application Settings here... from EEPROM 268 up to 511 (0 - 511)

} config;

void config_defaults() {
    Serial.println("Setting config Default values");

    config.DeviceName = String("Lamp");        // Device Name
    config.Location = String("Room");                 // Device Location
    config.ONTime = 60;                               // 0-255 seconds (Byte range)
    config.SLEEPTime = 5;                             // 0-255 minutes (Byte range)
    config.DEEPSLEEP = false;                         // 0 - Disabled, 1 - Enabled
    config.LED = false;                               // 0 - OFF, 1 - ON
    config.TELNET = false;                            // 0 - Disabled, 1 - Enabled
    config.OTA = false;                               // 0 - Disabled, 1 - Enabled
    config.WEB = false;                               // 0 - Disabled, 1 - Enabled
    config.STAMode = true;                            // 0 - AP Mode, 1 - Station Mode
//    config.ssid = "ESP8266-" + ChipID;                // SSID of access point
//    config.WiFiKey = "";                              // Password of access point
    config.ssid = String("ThomsonCasaN");             // Testing SSID
    config.WiFiKey = String("12345678");              // Testing Password
    config.dhcp = true;
    config.IP[0] = 192; config.IP[1] = 168; config.IP[2] = 1; config.IP[3] = 10;
    config.Netmask[0] = 255; config.Netmask[1] = 255; config.Netmask[2] = 255; config.Netmask[3] = 0;
    config.Gateway[0] = 192; config.Gateway[1] = 168; config.Gateway[2] = 1; config.Gateway[3] = 254;
    config.NTPServerName = String("pt.pool.ntp.org");
    config.Update_Time_Via_NTP_Every = 1200;          // Time in minutes
    config.TimeZone = 10;                             // -120 to 130 see Page_NTPSettings.h
    config.isDayLightSaving = 1;                      // 0 - Disabled, 1 - Enabled
    config.MQTT_Server = String("iothubna.hopto.org");// MQTT Broker Server (URL or IP)
    config.MQTT_Port = 1883;                          // MQTT Broker TCP port
    config.MQTT_User = String("admin");               // MQTT Broker username
    config.MQTT_Password = String("admin");           // MQTT Broker password
    config.MQTT_ClientID = String("001001");          // MQTT Client ID
}


#include <storage.h>
#include <global.h>
#include <wifi.h>
#include <telnet.h>
//#include <ntp.h>
//#include <web.h>
#include <ota.h>
#include <mqtt.h>





// **** Normal code functions here ...
void touched () {
    detachInterrupt(TouchInput);                        // to avoid this function call be retriggered (or watch dog bites!!)
    NOW = millis();
    if (NOW - last_touched > interval / 2) {        // respect minimum time between 2 consecutive function calls
        while (millis() - NOW < interval / 2) {}   // loop to allow reed status be stable before reading it
        if (digitalRead(TouchInput) == LOW) {
            BTN_touched = true;
            if (NOW - last_touched < 6 * interval) {
                COUNT_touched +=1;
                if (COUNT_touched==1) {
                    DOUBLE_touched = true;
                    telnet_println("TOUCH Button pressed twice!");
                };
                if (COUNT_touched==2) {
                    TRIPLE_touched = true;
                    telnet_println("TOUCH Button pressed 3 times!!!!");
                };
            }
            else {
                COUNT_touched =0;
                telnet_println("TOUCH Button pressed.");
            }
        }
        //telnet_println("Relay Status: " + String(RELAY_STATUS));  // this line crashes the ESP!!?!
    }
    last_touched = NOW;
    attachInterrupt(TouchInput, touched, FALLING);
}


void setup() {
// Output GPIOs
  pinMode(LED_esp, OUTPUT);
  pinMode(RELAY, OUTPUT);
  digitalWrite(LED_esp, HIGH);              // Turn LED off
  digitalWrite(RELAY, RELAY_STATUS);        // Turn RELAY off

// Input GPIOs
  pinMode(TouchInput,INPUT);
  attachInterrupt(TouchInput, touched, FALLING);


  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  Serial.println(" ");
  // Countdown waiting for platformIO to open the terminal window!
  //for (size_t i = 10 ; i > 0; i--) { Serial.print(String(i) + ", "); delay(1000); }

  Serial.println(" ");
  Serial.println("Hello World!");
  Serial.println("My ID is " + ChipID + " and I'm running version " + String(SWVer));
  Serial.println(ESP.getResetReason());

  // Start Storage service and read stored configuration
      storage_setup();

  // Start WiFi service (Station or as Acess Point)
      wifi_setup();

  // Start TELNET service
      if (config.TELNET) telnet_setup();

  // Start OTA service
      if (config.OTA) ota_setup();

  // Start ESP WEB Service
      //if (config.WEB) web_setup();

  // Start MQTT service
      int mqtt_status = mqtt_setup();
      mqtt_callback();

  // Start DHT device
      //dht_val.begin();

  // **** Normal SETUP Sketch code here...
  if (mqtt_status == MQTT_CONNECTED) {
      mqtt_publish(mqtt_pathtele(), "ChipID", ChipID);
      mqtt_publish(mqtt_pathtele(), "SWVer", String(SWVer));
      //mqtt_publish(mqtt_pathtele(), "BatLevel", String(getVoltage()));
      mqtt_publish(mqtt_pathtele(), "RSSI", String(getRSSI()));
      mqtt_publish(mqtt_pathtele(), "Status", "ON");

      mqtt_subscribe(mqtt_pathconf(), "+");
  }

} // end of setup()


void loop() {
  // allow background process to run.
      yield();

  // LED handling usefull if you need to identify the unit from many
      digitalWrite(LED_esp, boolean(!config.LED));  // Values is reversed due to Pull-UP configuration

  // TELNET handling
      if (config.TELNET) telnet_loop();

  // OTA request handling
      if (config.OTA) ota_loop();

  // ESP WEB Server request handling
      //if (config.WEB) web_loop();

  // MQTT handling
      mqtt_loop();


  // **** Normal LOOP Skecth code here ...
  // Normal Touch button to flip the Relay status
  if (BTN_touched && !DOUBLE_touched && !TRIPLE_touched && (millis() - last_touched > 6 * interval)) {
      RELAY_STATUS = !RELAY_STATUS;
      BTN_touched = false;
  }

  // Double touch button handling
  if (DOUBLE_touched && !TRIPLE_touched && (millis() - last_touched > 6 * interval)) {
      BTN_touched = false;
      DOUBLE_touched = false;
      flash_LED(2);
      mqtt_publish(mqtt_pathtele(), "DOUBLE_touched", "true");
      telnet_println("MQTT DOUBLE_touched message published.");
  }

  // Triple touch button handling
  if (TRIPLE_touched && (millis() - last_touched > 6 * interval)) {
      BTN_touched = false;
      DOUBLE_touched = false;
      TRIPLE_touched = false;
      flash_LED(3);
      mqtt_publish(mqtt_pathtele(), "TRIPLE_touched", "true");
      telnet_println("MQTT TRIPLE_touched message published.");
  }

  if (Last_STATUS != RELAY_STATUS) {
    digitalWrite(RELAY, RELAY_STATUS);        // Turn RELAY off
    mqtt_publish(mqtt_pathtele(), "Relay", String(RELAY_STATUS));
    Last_STATUS = RELAY_STATUS;
    Serial.println("RELAY status is: " + String(RELAY_STATUS));
  }
}  // end of loop()
