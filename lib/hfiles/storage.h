#include <EEPROM.h>

#define EEPROMZize 512

//
//  AUXILIAR functions to handle EEPROM
//

void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address) {
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void WriteStringToEEPROM(int beginaddress, String string) {
  char  charBuf[string.length() + 1];
  string.toCharArray(charBuf, string.length() + 1);
  for (int t =  0; t < sizeof(charBuf); t++)
  {
    EEPROM.write(beginaddress + t, charBuf[t]);
  }
}

String  ReadStringFromEEPROM(int beginaddress) {
  volatile byte counter = 0;
  char rChar;
  String retString = "";
  while (1)
  {
    rChar = EEPROM.read(beginaddress + counter);
    if (rChar == 0) break;
    if (counter > 31) break;
    counter++;
    retString.concat(rChar);

  }
  return retString;
}




//
// MAIN Functions
//

boolean storage_read() {
  Serial.println("Reading Configuration");
  if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F'  && EEPROM.read(2) == 'G' )
  {
    Serial.println("Configurarion Found!");
    config.DeviceName = ReadStringFromEEPROM(16);           // 16 Byte
    config.ONTime = EEPROM.read(37);                        // 1 Byte
    config.SLEEPTime = EEPROM.read(38);                     // 1 Byte
    config.DEEPSLEEP = boolean(EEPROM.read(39));            // 1 Byte
    config.LED = boolean(EEPROM.read(40));                  // 1 Byte
    config.TELNET = boolean(EEPROM.read(41));               // 1 Byte
    config.OTA = boolean(EEPROM.read(42));                  // 1 Byte
    config.WEB = boolean(EEPROM.read(43));                  // 1 Byte
    config.ssid = ReadStringFromEEPROM(44);                 // 16 Byte
    config.WiFiKey = ReadStringFromEEPROM(60);              // 16 Byte
    config.STAMode = boolean(EEPROM.read(76));       // TRUE - AP Mode; False - Client Mode
    config.dhcp =  EEPROM.read(77);
    config.IP[0] = EEPROM.read(80);
    config.IP[1] = EEPROM.read(81);
    config.IP[2] = EEPROM.read(82);
    config.IP[3] = EEPROM.read(83);
    config.Netmask[0] = EEPROM.read(84);
    config.Netmask[1] = EEPROM.read(85);
    config.Netmask[2] = EEPROM.read(86);
    config.Netmask[3] = EEPROM.read(87);
    config.Gateway[0] = EEPROM.read(88);
    config.Gateway[1] = EEPROM.read(89);
    config.Gateway[2] = EEPROM.read(90);
    config.Gateway[3] = EEPROM.read(91);
    config.NTPServerName = ReadStringFromEEPROM(128);       // 32 Byte
    config.TimeZone = EEPROMReadlong(160);                  //  4 Byte
    config.Update_Time_Via_NTP_Every = EEPROMReadlong(164); //  4 Byte
    config.isDayLightSaving = boolean(EEPROM.read(168));    //  1 Byte
    config.MQTT_Server = ReadStringFromEEPROM(176);         // 32 Byte
    config.MQTT_Port = EEPROMReadlong(208);                 //  4 Byte
    config.MQTT_User = ReadStringFromEEPROM(212);           // 16 Byte
    config.MQTT_Password = ReadStringFromEEPROM(228);       // 16 Byte
    config.MQTT_ClientID = ReadStringFromEEPROM(244);       //  8 Byte
    config.Location = ReadStringFromEEPROM(252);            // 16 Byte

    // Application parameters here ... from EEPROM 268 to 511


    return true;

  }
  else
  {
    Serial.println("Configurarion NOT FOUND!!!!");
    return false;
  }
}


void storage_print() {

  Serial.println("Printing Config");
  Serial.printf("Device Name: %s and Location: %s\n", config.DeviceName.c_str(), config.Location.c_str());
  Serial.printf("ON time: %d  SLEEP Time: %d  DEEPSLEEP enabled: %d\n", config.ONTime, config.SLEEPTime, config.DEEPSLEEP);
  Serial.printf("LED enabled: %d - TELNET enabled: %d - OTA enabled: %d - WEB enabled: %d\n", config.LED, config.TELNET, config.OTA, config.WEB);
  Serial.printf("WiFi STA Mode: %d\n", config.STAMode);
  Serial.printf("WiFi SSID: %s\n", config.ssid.c_str());
  Serial.printf("WiFi Key: %s\n", config.WiFiKey.c_str());

  Serial.printf("DHCP enabled: %d\n", config.dhcp);
  if(!config.dhcp) {
      Serial.printf("IP: %d.%d.%d.%d\n", config.IP[0],config.IP[1],config.IP[2],config.IP[3]);
      Serial.printf("Mask: %d.%d.%d.%d\n", config.Netmask[0],config.Netmask[1],config.Netmask[2],config.Netmask[3]);
      Serial.printf("Gateway: %d.%d.%d.%d\n", config.Gateway[0],config.Gateway[1],config.Gateway[2],config.Gateway[3]);
  }
  Serial.printf("NTP Server Name: %s\n", config.NTPServerName.c_str());
  Serial.printf("NTP update every %ld minutes.\n", config.Update_Time_Via_NTP_Every);
  Serial.printf("Timezone: %ld  -  DayLight: %d\n", config.TimeZone, config.isDayLightSaving);


    // Application Settings here... from EEPROM 268 up to 511 (0 - 511)

}



void storage_write() {

  Serial.println("Writing Config");
  EEPROM.write(0, 'C');
  EEPROM.write(1, 'F');
  EEPROM.write(2, 'G');

  WriteStringToEEPROM(16, config.DeviceName);
  EEPROM.write(37, config.ONTime);                        // 1 Byte
  EEPROM.write(38, config.SLEEPTime);                     // 1 Byte
  EEPROM.write(39, config.DEEPSLEEP);                     // 1 Byte
  EEPROM.write(40, config.LED);                           // 1 Byte
  EEPROM.write(41, config.TELNET);                        // 1 Byte
  EEPROM.write(42, config.OTA);                           // 1 Byte
  EEPROM.write(43, config.WEB);                           // 1 Byte

  WriteStringToEEPROM(44, config.ssid);
  WriteStringToEEPROM(60, config.WiFiKey);
  EEPROM.write(76, config.STAMode);
  EEPROM.write(77, config.dhcp);

  EEPROM.write(80, config.IP[0]);
  EEPROM.write(81, config.IP[1]);
  EEPROM.write(82, config.IP[2]);
  EEPROM.write(83, config.IP[3]);

  EEPROM.write(84, config.Netmask[0]);
  EEPROM.write(85, config.Netmask[1]);
  EEPROM.write(86, config.Netmask[2]);
  EEPROM.write(87, config.Netmask[3]);

  EEPROM.write(88, config.Gateway[0]);
  EEPROM.write(89, config.Gateway[1]);
  EEPROM.write(90, config.Gateway[2]);
  EEPROM.write(91, config.Gateway[3]);

  WriteStringToEEPROM(128, config.NTPServerName);
  EEPROMWritelong(160, config.TimeZone);                    // 4 Byte
  EEPROMWritelong(164, config.Update_Time_Via_NTP_Every);   // 4 Byte
  EEPROM.write(168, config.isDayLightSaving);

  WriteStringToEEPROM(176, config.MQTT_Server);             // 32 Byte
  EEPROMWritelong(208, config.MQTT_Port);                   //  4 Byte
  WriteStringToEEPROM(212, config.MQTT_User);               // 16 Byte
  WriteStringToEEPROM(228, config.MQTT_Password);           // 16 Byte
  WriteStringToEEPROM(244, config.MQTT_ClientID);           //  8 Byte
  WriteStringToEEPROM(252, config.Location);                // 16 Byte

    // Application Settings here... from EEPROM 256 up to 511 (0 - 511)

  EEPROM.commit();
}

void storage_reset() {

  Serial.println("Reseting Config");
  EEPROM.write(0, 'R');
  EEPROM.write(1, 'S');
  EEPROM.write(2, 'T');
  for (size_t i = 3; i < (EEPROMZize-1); i++) {
    EEPROM.write(i, 0);
  }

  EEPROM.commit();
}

void storage_setup() {
    bool CFG_saved = false;
    EEPROM.begin(EEPROMZize);     // define an EEPROM space of 512Bytes to store data
    //storage_reset();            // Hack to reset storage during boot
    CFG_saved = storage_read();   //When a configuration exists, it uses stored values
    if (!CFG_saved) {             // If NOT, it Set DEFAULT VALUES to "config" struct
        config_defaults();
        storage_write();
    }
    storage_print();
}