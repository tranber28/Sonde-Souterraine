#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <lmic.h>
#include <hal/hal.h>
#include <esp_sleep.h>

// ─── CONFIGURATION PINS ───
#define PIN_DS18B20      13    
#define PIN_VCC_TEMP     14    
#define PIN_VCC_MOIST    25    
#define PIN_MOIST_10     34    
#define PIN_MOIST_30     35    
#define PIN_MOIST_60     39    

// ─── OBJETS ───
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);
#define MY_OLED_RST  4
Adafruit_SSD1306 display(128, 64, &Wire, MY_OLED_RST);

// ─── LORA PINS (LilyGO T3 V1.6.1) ───
const lmic_pinmap lmic_pins = {
  .nss  = 18, .rxtx = LMIC_UNUSED_PIN, .rst  = 23, .dio  = { 26, 33, 32 },
};

// ─── IDENTIFIANTS CHIRPSTACK (LSB/MSB configurés) ───
static const u1_t PROGMEM DEVEUI[8]  = { 0xa8, 0x52, 0x6f, 0x34, 0x31, 0x60, 0xb4, 0x5c };
static const u1_t PROGMEM APPEUI[8]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPKEY[16] = { 0x2c, 0x45, 0xb9, 0x7e, 0x77, 0x47, 0x94, 0x6f, 0x97, 0xce, 0xbc, 0x1d, 0x0e, 0xac, 0x5f, 0x12 };

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

static osjob_t sendjob;
#define SEND_INTERVAL_S 3600UL 

float t1=0, t2=0, t3=0;
int m10=0, m30=0, m60=0;

// ─── FONCTION AFFICHAGE ───
void updateDisplay(const char* status) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("--- STATION COGGIA ---");
  display.printf("T: %.1f | %.1f | %.1f\n", t1, t2, t3);
  display.println("----------------------");
  display.printf("H 10cm: %d %%\n", m10);
  display.printf("H 30cm: %d %%\n", m30);
  display.printf("H 60cm: %d %%\n", m60);
  display.setCursor(0, 55);
  display.print(status);
  display.display();
}

void readSensors() {
  digitalWrite(PIN_VCC_TEMP, HIGH);
  digitalWrite(PIN_VCC_MOIST, HIGH);
  delay(800); 

  sensors.begin();
  sensors.requestTemperatures();
  t1 = sensors.getTempCByIndex(0);
  t2 = sensors.getTempCByIndex(1);
  t3 = sensors.getTempCByIndex(2);

  // Calibration avec tes mesures réelles
  m10 = map(analogRead(PIN_MOIST_10), 2975, 1400, 0, 100);
  m30 = map(analogRead(PIN_MOIST_30), 2470, 1400, 0, 100); 
  m60 = map(analogRead(PIN_MOIST_60), 2960, 1400, 0, 100);

  m10 = constrain(m10, 0, 100); 
  m30 = constrain(m30, 0, 100); 
  m60 = constrain(m60, 0, 100);
  
  digitalWrite(PIN_VCC_TEMP, LOW);
  digitalWrite(PIN_VCC_MOIST, LOW);
}

void do_send(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) return;
  readSensors();
  updateDisplay("ENVOI LORA...");

  uint8_t payload[6];
  payload[0] = (int8_t)(t1 * 10);
  payload[1] = (int8_t)(t2 * 10);
  payload[2] = (int8_t)(t3 * 10);
  payload[3] = (uint8_t)m10;
  payload[4] = (uint8_t)m30;
  payload[5] = (uint8_t)m60;

  LMIC_setTxData2(1, payload, sizeof(payload), 0);
}

void onEvent(ev_t ev) {
  switch(ev) {
    case EV_JOINING:
      updateDisplay("JOINING...");
      break;
    case EV_JOINED: 
      updateDisplay("JOINED !");
      LMIC_setAdrMode(1); 
      os_setCallback(&sendjob, do_send); 
      break;
    case EV_TXCOMPLETE: 
      updateDisplay("OK! SOMMEIL 1H");
      delay(15000); // RESTE ALLUMÉ 15 SECONDES POUR LIRE
      display.ssd1306_command(SSD1306_DISPLAYOFF); // Éteint l'écran physiquement
      esp_sleep_enable_timer_wakeup((uint64_t)SEND_INTERVAL_S * 1000000ULL);
      esp_deep_sleep_start();
      break;
    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_VCC_TEMP, OUTPUT);
  pinMode(PIN_VCC_MOIST, OUTPUT);
  
  pinMode(MY_OLED_RST, OUTPUT);
  digitalWrite(MY_OLED_RST, LOW); delay(20); digitalWrite(MY_OLED_RST, HIGH);
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();

  SPI.begin(5, 19, 27, 18);
  os_init();
  LMIC_reset();
  LMIC_startJoining();
}

void loop() { 
  os_runloop_once(); 
}
