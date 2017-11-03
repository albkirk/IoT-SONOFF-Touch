#include <DHT.h>


// Battery Voltage
#define vcc_corr float(0.82)               // ESP Voltage corrective Factor,  due to diode voltage drop + any electronic self heat behaviour.
#define vcc_thrs float(3.5)               // Battery lowest voltage (ESP voltage input --> so, after LDO circuit)  before slepping forever.
String voltage = "9.99";                  // Voltage variable in string format

// Initialize DHT sensor.
#define DHTPIN 5                          // Connected to PIN GPIO5
#define DHTTYPE DHT22                     // using the DHT22 Model
#define tem_corr float(-1)                // DHT Temperature corrective Factor, tipically due to electronic self heat behaviour.
#define samp_intv int(300)                // Temperature and Humidity Sampling Interval value in seconds
unsigned long curr_time;                  // Variable used to write the current time
unsigned long last_sample;                // Variable used to write time the last sample was taken
DHT dht_val(DHTPIN, DHTTYPE);

// Timers for millis used on Sleeping and LED flash
unsigned long Extend_time=0;
unsigned long now_millis=0;
unsigned long Pace_millis=3000;
unsigned long LED_millis=300;             // 10 slots available (3000 / 300)
unsigned long BUZZER_millis=500;            // 6 Buzz beeps maximum  (3000 / 500)

// Functions //
float getVoltage (){
  float vdd = 0;

  do {
      delay(100);
      vdd = (analogRead(A0) / 1000.0) * 3.3 + vcc_corr;
      // Serial.print("VDD value= "); Serial.println(String(vdd));
  } while (vdd < 2);               // the ESP do not run with a supply voltage lower than 2v!! So, if it is measuring it, it's not true!
  return vdd;
}

long getRSSI() {
    // Read WiFi RSSI Strength signal

  long r = 0;
  int n = 0;

  while (n < 10) {
    r += WiFi.RSSI();
    n ++;
    }
  r = r /10;
  return r;
}


float getTemperature() {
    // Read temperature as Celsius (the default)

  float t;
  int n = 0;

  while (n < 10) {
    t = dht_val.readTemperature() + tem_corr;
    // Check if any reads failed and exit.
    if (isnan(t)) {
      Serial.println("Failed to read temperature from DHT sensor!");
      delay(1000);
      //t = NULL;
      n ++;
    }
    else {
      //Serial.print(t);
      return t;
    }
  }

}


float getHumidity() {
    // Read Humidity as Percentage (the default)

  float h;
  int n = 0;

  while (n < 10 ) {
    h = dht_val.readHumidity();
    // Check if any reads failed and exit.
    if (isnan(h)) {
      Serial.println("Failed to read humidity from DHT sensor!");
      delay(1000);
      //h = NULL;
      n ++;
    }
    else {
      //Serial.print(h);
      return h;
    }
  }

}

void BootESP() {
  Serial.println("Booting in 3 seconds...");
  delay(3000);
  ESP.restart();
}

void FormatConfig() {     // WARNING!! To be used only as last resource!!!
    Serial.println(ESP.eraseConfig());
    delay(5000);
    ESP.reset();
}

void blink_LED(int slot) {    // slot range 1 to 10 =>> 3000/300
    now_millis = millis() % Pace_millis;
    if (now_millis > LED_millis*(slot-1) && now_millis < LED_millis*slot ) {
        digitalWrite(LED_esp, boolean(config.LED));  // toggles LED status. will be restored by command above
        delay(LED_millis/2);
    }
}

void flash_LED(int n_flash) {    // number of flash 1 to 6 =>> 3000/500
    for (size_t i = 0; i < n_flash; i++) {
      digitalWrite(LED_esp, boolean(config.LED));      // Turn LED on
      delay(LED_millis/3);
      digitalWrite(LED_esp, boolean(!config.LED));     // Turn LED off
      delay(LED_millis/3);
    }
}

void Buzz(int n_beeps) {    // number of beeps 1 to 6 =>> 3000/500
    for (size_t i = 0; i < n_beeps; i++) {
      digitalWrite(BUZZER, HIGH);      // Turn Buzzer on
      delay(BUZZER_millis/2);
      digitalWrite(BUZZER, LOW);      // Turn Buzzer off
      delay(BUZZER_millis/2);
    }
}
