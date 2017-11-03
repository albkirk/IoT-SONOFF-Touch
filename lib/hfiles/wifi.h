// Expose Espressif SDK functionality
extern "C" {
#include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

//#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

// Sniffer CONSTANTs
#define ETH_MAC_LEN 6
#define MAX_APS_TRACKED 100
#define MAX_CLIENTS_TRACKED 200
#define PURGETIME 600000
#define MINRSSI -70
#define MAXDEVICES 60
#define JBUFFER 15+ (MAXDEVICES * 40)
#define SENDTIME 30000
WiFiClient wifiClient;

uint8_t broadcast1[3] = { 0x01, 0x00, 0x5e };
uint8_t broadcast2[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uint8_t broadcast3[3] = { 0x33, 0x33, 0x00 };


// Sniffer STRUCTUREs
struct beaconinfo
{
  uint8_t bssid[ETH_MAC_LEN];
  uint8_t ssid[33];
  int ssid_len;
  int channel;
  int err;
  signed rssi;
  uint8_t capa[2];
  long lastDiscoveredTime;
};

struct clientinfo
{
  uint8_t bssid[ETH_MAC_LEN];
  uint8_t station[ETH_MAC_LEN];
  uint8_t ap[ETH_MAC_LEN];
  int channel;
  int err;
  signed rssi;
  uint16_t seq_n;
  long lastDiscoveredTime;
};

/* ==============================================
   Promiscous callback structures, see ESP manual
   ============================================== */
struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;
  unsigned: 12;
};

struct LenSeq {
  uint16_t length;
  uint16_t seq;
  uint8_t  address3[6];
};

struct sniffer_buf {
  struct RxControl rx_ctrl;
  uint8_t buf[36];
  uint16_t cnt;
  struct LenSeq lenseq[1];
};

struct sniffer_buf2 {
  struct RxControl rx_ctrl;
  uint8_t buf[112];
  uint16_t cnt;
  uint16_t len;
};

struct clientinfo parse_data(uint8_t *frame, uint16_t framelen, signed rssi, unsigned channel)
{
  struct clientinfo ci;
  ci.channel = channel;
  ci.err = 0;
  ci.rssi = rssi;
  int pos = 36;
  uint8_t *bssid;
  uint8_t *station;
  uint8_t *ap;
  uint8_t ds;

  ds = frame[1] & 3;    //Set first 6 bits to 0
  switch (ds) {
    // p[1] - xxxx xx00 => NoDS   p[4]-DST p[10]-SRC p[16]-BSS
    case 0:
      bssid = frame + 16;
      station = frame + 10;
      ap = frame + 4;
      break;
    // p[1] - xxxx xx01 => ToDS   p[4]-BSS p[10]-SRC p[16]-DST
    case 1:
      bssid = frame + 4;
      station = frame + 10;
      ap = frame + 16;
      break;
    // p[1] - xxxx xx10 => FromDS p[4]-DST p[10]-BSS p[16]-SRC
    case 2:
      bssid = frame + 10;
      // hack - don't know why it works like this...
      if (memcmp(frame + 4, broadcast1, 3) || memcmp(frame + 4, broadcast2, 3) || memcmp(frame + 4, broadcast3, 3)) {
        station = frame + 16;
        ap = frame + 4;
      } else {
        station = frame + 4;
        ap = frame + 16;
      }
      break;
    // p[1] - xxxx xx11 => WDS    p[4]-RCV p[10]-TRM p[16]-DST p[26]-SRC
    case 3:
      bssid = frame + 10;
      station = frame + 4;
      ap = frame + 4;
      break;
  }

  memcpy(ci.station, station, ETH_MAC_LEN);
  memcpy(ci.bssid, bssid, ETH_MAC_LEN);
  memcpy(ci.ap, ap, ETH_MAC_LEN);

  ci.seq_n = frame[23] * 0xFF + (frame[22] & 0xF0);
  return ci;
}

struct beaconinfo parse_beacon(uint8_t *frame, uint16_t framelen, signed rssi)
{
  struct beaconinfo bi;
  bi.ssid_len = 0;
  bi.channel = 0;
  bi.err = 0;
  bi.rssi = rssi;
  int pos = 36;

  if (frame[pos] == 0x00) {
    while (pos < framelen) {
      switch (frame[pos]) {
        case 0x00: //SSID
          bi.ssid_len = (int) frame[pos + 1];
          if (bi.ssid_len == 0) {
            memset(bi.ssid, '\x00', 33);
            break;
          }
          if (bi.ssid_len < 0) {
            bi.err = -1;
            break;
          }
          if (bi.ssid_len > 32) {
            bi.err = -2;
            break;
          }
          memset(bi.ssid, '\x00', 33);
          memcpy(bi.ssid, frame + pos + 2, bi.ssid_len);
          bi.err = 0;  // before was error??
          break;
        case 0x03: //Channel
          bi.channel = (int) frame[pos + 2];
          pos = -1;
          break;
        default:
          break;
      }
      if (pos < 0) break;
      pos += (int) frame[pos + 1] + 2;
    }
  } else {
    bi.err = -3;
  }

  bi.capa[0] = frame[34];
  bi.capa[1] = frame[35];
  memcpy(bi.bssid, frame + 10, ETH_MAC_LEN);
  return bi;
}



// WiFi and Sniffer VARIABLEs
int WIFI_state = WL_DISCONNECTED;

int aps_known_count_old, aps_known_count = 0;             // Number of known APs
int clients_known_count_old, clients_known_count = 0;     // Number of known CLIENTs
beaconinfo aps_known[MAX_APS_TRACKED];                    // Array to save MACs of known APs
clientinfo clients_known[MAX_CLIENTS_TRACKED];            // Array to save MACs of known CLIENTs

unsigned long sendEntry;
char jsonString[JBUFFER];
StaticJsonBuffer<JBUFFER>  wifijsonBuffer;



// Sniffer FUNCTIONS
String formatMac1(uint8_t mac[ETH_MAC_LEN]) {
  String hi = "";
  for (int i = 0; i < ETH_MAC_LEN; i++) {
    if (mac[i] < 16) hi = hi + "0" + String(mac[i], HEX);
    else hi = hi + String(mac[i], HEX);
    if (i < 5) hi = hi + ":";
  }
  return hi;
}

int register_beacon(beaconinfo beacon)
{
  int known = 0;   // Clear known flag
  for (int u = 0; u < aps_known_count; u++)
  {
    if (! memcmp(aps_known[u].bssid, beacon.bssid, ETH_MAC_LEN)) {
      aps_known[u].lastDiscoveredTime = millis();
      aps_known[u].rssi = beacon.rssi;
      known = 1;
      break;
    }   // AP known => Set known flag
  }
  if (! known && (beacon.err == 0))  // AP is NEW, copy MAC to array and return it
  {
    beacon.lastDiscoveredTime = millis();
    memcpy(&aps_known[aps_known_count], &beacon, sizeof(beacon));
    /*    Serial.print("Register Beacon ");
        Serial.print(formatMac1(beacon.bssid));
        Serial.print(" Channel ");
        Serial.print(aps_known[aps_known_count].channel);
        Serial.print(" RSSI ");
        Serial.println(aps_known[aps_known_count].rssi);*/

    aps_known_count++;

    if ((unsigned int) aps_known_count >=
        sizeof (aps_known) / sizeof (aps_known[0]) ) {
      Serial.printf("exceeded max aps_known\n");
      aps_known_count = 0;
    }
  }
  return known;
}

int register_client(clientinfo ci) {
  int known = 0;   // Clear known flag
  for (int u = 0; u < clients_known_count; u++)
  {
    if (! memcmp(clients_known[u].station, ci.station, ETH_MAC_LEN)) {
      clients_known[u].lastDiscoveredTime = millis();
      clients_known[u].rssi = ci.rssi;
      known = 1;
      break;
    }
  }
  if (! known) {
    ci.lastDiscoveredTime = millis();
    // search for Assigned AP
    for (int u = 0; u < aps_known_count; u++) {
      if (! memcmp(aps_known[u].bssid, ci.bssid, ETH_MAC_LEN)) {
        ci.channel = aps_known[u].channel;
        break;
      }
    }
    if (ci.channel != 0) {
      memcpy(&clients_known[clients_known_count], &ci, sizeof(ci));
      /*   Serial.println();
         Serial.print("Register Client ");
         Serial.print(formatMac1(ci.station));
         Serial.print(" Channel ");
         Serial.print(ci.channel);
         Serial.print(" RSSI ");
         Serial.println(ci.rssi);*/

      clients_known_count++;
    }

    if ((unsigned int) clients_known_count >=
        sizeof (clients_known) / sizeof (clients_known[0]) ) {
      Serial.printf("exceeded max clients_known\n");
      clients_known_count = 0;
    }
  }
  return known;
}


String print_beacon(beaconinfo beacon)
{
  String hi = "";
  if (beacon.err != 0) {
    //Serial.printf("BEACON ERR: (%d)  ", beacon.err);
  } else {
    Serial.printf(" BEACON: <=============== [%32s]  ", beacon.ssid);
    Serial.print(formatMac1(beacon.bssid));
    Serial.printf("   %2d", beacon.channel);
    Serial.printf("   %4d\r\n", beacon.rssi);
  }
  return hi;
}

String print_client(clientinfo ci)
{
  String hi = "";
  int u = 0;
  int known = 0;   // Clear known flag
  if (ci.err != 0) {
    // nothing
  } else {
    Serial.printf("CLIENT: ");
    Serial.print(formatMac1(ci.station));  //Mac of device
    Serial.printf(" ==> ");

    for (u = 0; u < aps_known_count; u++)
    {
      if (! memcmp(aps_known[u].bssid, ci.bssid, ETH_MAC_LEN)) {
        //       Serial.print("   ");
        //        Serial.printf("[%32s]", aps_known[u].ssid);   // Name of connected AP
        known = 1;     // AP known => Set known flag
        break;
      }
    }

    if (! known)  {
      Serial.printf("   Unknown/Malformed packet \r\n");
      for (int i = 0; i < 6; i++) Serial.printf("%02x", ci.bssid[i]);
    } else {
      //    Serial.printf("%2s", " ");

      Serial.print(formatMac1(ci.ap));   // Mac of connected AP
      Serial.printf("  % 3d", aps_known[u].channel);  //used channel
      Serial.printf("   % 4d\r\n", ci.rssi);
    }
  }
  return hi;
}

void promisc_cb(uint8_t *buf, uint16_t len)
{
  int i = 0;
  uint16_t seq_n_new = 0;
  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
    if (register_beacon(beacon) == 0) {
      print_beacon(beacon);
    }
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    //Is data or QOS?
    if ((sniffer->buf[0] == 0x08) || (sniffer->buf[0] == 0x88)) {
      struct clientinfo ci = parse_data(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);
      if (memcmp(ci.bssid, ci.station, ETH_MAC_LEN)) {
        if (register_client(ci) == 0) {
          print_client(ci);
        }
      }
    }
  }
}


void purgeDevices() {
  for (int u = 0; u < clients_known_count; u++) {
    if ((millis() - clients_known[u].lastDiscoveredTime) > PURGETIME) {
      Serial.print("purged Client: " );
      Serial.println(formatMac1(clients_known[u].station));
      for (int i = u; i < clients_known_count; i++) memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
      clients_known_count--;
      clients_known_count_old--;
      break;
    }
  }
  for (int u = 0; u < aps_known_count; u++) {
    if ((millis() - aps_known[u].lastDiscoveredTime) > PURGETIME) {
      Serial.print("purged AP: " );
      Serial.println(formatMac1(aps_known[u].bssid));
      for (int i = u; i < aps_known_count; i++) memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
      aps_known_count--;
      aps_known_count_old--;
      break;
    }
  }
}


void wifi_showAPs() {
    // Show APs
    Serial.println("");
    Serial.println("-------------------AP DB-------------------");
    Serial.println(aps_known_count);
    for (int u = 0; u < aps_known_count; u++) {
        Serial.print("AP ");
        Serial.print(formatMac1(aps_known[u].bssid));
        Serial.print(" RSSI ");
        Serial.print(aps_known[u].rssi);
        Serial.print(" channel ");
        Serial.print(aps_known[u].channel);
        Serial.print(" SSID ");
        Serial.printf(" %32s\n  ", aps_known[u].ssid);
    }
    Serial.println("");
}

void wifi_showSTAs() {
    // Show Stations
    Serial.println("");
    Serial.println("-------------------Station DB-------------------");
    Serial.println(clients_known_count);
    for (int u = 0; u < clients_known_count; u++) {
        Serial.print("STA ");
        Serial.print(formatMac1(clients_known[u].station));
        Serial.print(" RSSI ");
        Serial.print(clients_known[u].rssi);
        Serial.print(" channel ");
        Serial.println(clients_known[u].channel);
    }
}

String wifi_listSTAs() {
    String devices;

    // Purge and recreate json string
    wifijsonBuffer.clear();
    JsonObject& root = wifijsonBuffer.createObject();
    JsonArray& sta = root.createNestedArray("Stations");

    // Add Clients that has RSSI higher then minimum
    for (int u = 0; u < clients_known_count; u++) {
        devices = formatMac1(clients_known[u].station);
        if (clients_known[u].rssi > MINRSSI) {
            sta.add(devices);
        }
    }

    root.prettyPrintTo(Serial);
    root.printTo(jsonString);
    Serial.print("jsonString ready to Publish: "); Serial.println((jsonString));
    return jsonString;
}

String wifi_listAPs() {
    String devices_ap, devices_ssid;

    // Purge and recreate json string
    wifijsonBuffer.clear();
    JsonObject& root = wifijsonBuffer.createObject();
    JsonArray& ap = root.createNestedArray("APs");
    JsonArray& ssid = root.createNestedArray("AP_SSIDs");

    // add APs
    for (int u = 0; u < aps_known_count; u++) {
        if (aps_known[u].rssi > MINRSSI) {
            devices_ap = formatMac1(aps_known[u].bssid);
            devices_ssid = *aps_known[u].ssid;
            ap.add(devices_ap);
            ssid.add(devices_ssid);
        }
    }

    root.prettyPrintTo(Serial);
    root.printTo(jsonString);
    Serial.print("jsonString ready to Publish: "); Serial.println((jsonString));
    return jsonString;
}


// Wi-Fi functions
void wifi_connect() {
  //  Connect to WiFi acess point or start as Acess point
  if (config.STAMode) {
      // Setup ESP8266 in Station mode
      WiFi.mode(WIFI_STA);
      delay(1000);
      //WiFi.begin("ThomsonCasaN", "12345678");
      WiFi.begin(config.ssid.c_str(), config.WiFiKey.c_str());
      WIFI_state = WiFi.waitForConnectResult();
  }
  if ( WIFI_state == WL_CONNECTED ) {
      Serial.print("Connected to WiFi network! " + config.ssid + " IP: "); Serial.println(WiFi.localIP());
  }
  else
  {
      Serial.println("WiFi STATUS: " + String(WiFi.status()));
      // Initialize Wifi in AP mode
      WiFi.mode(WIFI_AP);
      //WiFi.softAP("testess");
      WiFi.softAP(config.ssid.c_str());
      Serial.print("WiFi in AP mode, with IP: "); Serial.println(WiFi.softAPIP());
  }
}


void wifi_setup() {
    wifi_connect();
}


void wifi_sniffer() {
    wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
    wifi_set_channel(1);
    wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
    wifi_promiscuous_enable(true);
    boolean NewDevice = false;
    for (uint channel = 1; channel <= 13; channel++) {    // ESP only supports 1 ~ 13
        wifi_set_channel(channel);
        for (int n = 0; n < 200; n++) {   // 200 times delay(1) = 200 ms, which is 2 beacon's of 100ms
            delay(1);  // critical processing timeslice for NONOS SDK!
            if (clients_known_count > clients_known_count_old) {
                clients_known_count_old = clients_known_count;
                NewDevice = true;
            }
            if (aps_known_count > aps_known_count_old) {
                aps_known_count_old = aps_known_count;
                NewDevice = true;
            }
        }
    }
    purgeDevices();
    if (NewDevice) {
        wifi_showAPs();
        wifi_showSTAs();
    }
    wifi_promiscuous_enable(false);
    wifi_connect();
}
