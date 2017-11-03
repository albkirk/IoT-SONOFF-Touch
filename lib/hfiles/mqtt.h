#include <PubSubClient.h>
#include <ArduinoJson.h>

// MQTT PATH Structure
// /clientid/location/deviceName/telemetry/<topic>    --> typically, used when publishing info/status
// /clientid/location/deviceName/configure/<topic>    --> typically, used when subscribing for actions

// EXAMPLEs
// /001001/room/Estore/telemetry/RSSI                 --> value in dBm
// /001001/kitchen/AmbiSense/telemetry/BatLevel       --> 0 - 100
// /001001/outside/MailBox/telemetry/Status           --> OK / LOWBat
// /001001/kitchen/AmbSense/telemetry/DeepSleep       --> time in sec
// /001001/outside/MailBox/telemetry/LED              --> True / False  to turn LED ON/OFF
// /001001/outside/MailBox/configure/LED              --> Set True / False to turn LED ON/OFF

// MQTT Constants
//#define MQTT_MAX_PACKET_SIZE 2048

// Initialize MQTT Client
PubSubClient MQTTclient(wifiClient);


// MQTT Functions //
String mqtt_pathtele() {
  return "/" + config.MQTT_ClientID + "/" + config.Location + "/" + config.DeviceName + "/telemetry/";
}

String mqtt_pathconf() {
  return "/" + config.MQTT_ClientID + "/" + config.Location + "/" + config.DeviceName + "/configure/";
}

int mqtt_setup() {
      Serial.print("Connecting to MQTT Broker ...");
      if ( WiFi.status() == !WL_CONNECTED ) wifi_connect();
      MQTTclient.setServer(config.MQTT_Server.c_str(), config.MQTT_Port);
      // Attempt to connect (clientId, mqtt username, mqtt password)
      if ( MQTTclient.connect(config.MQTT_ClientID.c_str(), config.MQTT_User.c_str(), config.MQTT_Password.c_str())) {
          Serial.println( "[DONE]" );
      }
      else {
          Serial.print( "MQTT ERROR! ==> " );
          Serial.println( MQTTclient.state() );
        }
return MQTTclient.state();
}

// MQTT commands to run on loop function.
void mqtt_loop() {
    MQTTclient.loop();
    yield();
}

void mqtt_publish(String pubpath, String pubtopic, String pubvalue) {
  String topic = "";
  topic += pubpath; topic += pubtopic;     //topic += "/";
  // Send payload
  if (MQTTclient.publish(topic.c_str(), pubvalue.c_str()) == 1) Serial.printf("MQTT published:  %s = %s\n", topic.c_str(), pubvalue.c_str());
  else {
      flash_LED(5);
      Serial.println("MQTT message NOT published.");
      if ( WiFi.status() != WL_CONNECTED || MQTTclient.state() != MQTT_CONNECTED ) mqtt_setup();
      else {
          Serial.println();
          Serial.println("!!!!! Not published. Try uncomment #define MQTT_MAX_PACKET_SIZE 2048 at the beginning of mqtt.h file");
          Serial.println();
      }
  }




}

void mqtt_subscribe(String subpath, String subtopic) {

  String topic = "";
  topic += subpath; topic += subtopic; //topic += "/";

  MQTTclient.subscribe(topic.c_str());

  Serial.println("subscribed to topic: " + topic);
}

// Handling of PUBLISHed message
void on_message(const char* topic, byte* payload, unsigned int length) {
  // allow background process to run.
  yield();

  Serial.println("New message received from Broker");

  char msg[length + 1];
  strncpy (msg, (char*)payload, length);
  msg[length] = '\0';

  Serial.println("Topic: " + String(topic));
  Serial.println("Payload: " + String((char*)msg));

  // Decode JSON request
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)msg);

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  // Check request method
  String reqparam = String((const char*)data["param"]);
  String reqvalue = String((const char*)data["value"]);
  Serial.println("Received Data: " + reqparam + " = " + reqvalue);


  if ( reqparam == "DeviceName") config.DeviceName = String((const char*)data["value"]);
  if ( reqparam == "Location") config.Location = String((const char*)data["value"]);
  if ( reqparam == "ONTime") config.ONTime = data["value"];
  if ( reqparam == "SLEEPTime") config.SLEEPTime = data["value"];
  if ( reqparam == "DEEPSLEEP") { config.DEEPSLEEP = data["value"]; storage_write(); BootESP(); }
  if ( reqparam == "LED") config.LED = data["value"];
  if ( reqparam == "TELNET") { config.TELNET = data["value"]; storage_write(); BootESP(); }
  if ( reqparam == "OTA") { config.OTA = data["value"]; storage_write(); BootESP(); }
  if ( reqparam == "WEB") { config.WEB = data["value"]; storage_write(); BootESP(); }
  if ( reqparam == "STAMode") config.STAMode = data["value"];
  if ( reqparam == "ssid") config.ssid = String((const char*)data["value"]);
  if ( reqparam == "WiFiKey") config.WiFiKey = String((const char*)data["value"]);
  if ( reqparam == "NTPServerName") config.NTPServerName = String((const char*)data["value"]);
  if ( reqparam == "Update_Time_Via_NTP_Every") config.Update_Time_Via_NTP_Every = data["value"];
  if ( reqparam == "TimeZone") config.TimeZone = data["value"];
  if ( reqparam == "isDayLightSaving") config.isDayLightSaving = data["value"];
  if ( reqparam == "Store") if (data["value"] == true) storage_write();
  if ( reqparam == "Boot") if (data["value"] == true) BootESP();
  if ( reqparam == "Reset") if (data["value"] == true) storage_reset();
  if ( reqparam == "RELAY") RELAY_STATUS = data["value"];

  storage_print();
  Extend_time=60;
}

// The callback for when a PUBLISH message is received from the server.
void mqtt_callback() {
    MQTTclient.setCallback(on_message);
}
