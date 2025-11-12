#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <ScioSense_ENS160.h>
#include <RTClib.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#define OLED_ADDR 0x3C
SSD1306AsciiWire oled;

#define SD_CS_PIN 10
ScioSense_ENS160 ens160(ENS160_I2CADDR_1);

// Fresh file each boot to avoid mixing formats
const char *LOG_FILE = "iaq_log.csv";

unsigned long lastSample = 0;
const unsigned long SAMPLE_MS = 10000;

RTC_DS3231 rtc;
#define ENA 9
#define IN1 8
#define IN2 7
int fanSpeed = 0;

const uint8_t PIN_OUTSIDE = 2;
const uint8_t PIN_INSIDE  = 3;
const bool ACTIVE_LOW = true;
const unsigned long EDGE_DEBOUNCE_MS = 40;
const unsigned long SEQ_TIMEOUT_MS   = 2000;
int peopleCount = 0;

// LED pins (one ON at a time)
#define LED_GOOD 5   // ON when fan OFF
#define LED_BAD  6   // ON when fan ON

struct Debounced { uint8_t pin; bool raw; bool stable; unsigned long tchg; };
Debounced sOutside{PIN_OUTSIDE,false,false,0};
Debounced sInside {PIN_INSIDE,false,false,0};

enum SeqState { IDLE, SEEN_OUTSIDE, SEEN_INSIDE, WAIT_BOTH_CLEAR };
SeqState seq = IDLE;
unsigned long tSeqStart = 0;
inline bool isActive(bool l){return ACTIVE_LOW?l==LOW:l==HIGH;}
bool outsideActivePrev=false,insideActivePrev=false;

// OLED page state
String lastLine = "";
bool lastPage = true;

// ---------- OLED functions ----------
void showPage1(uint8_t aqi,uint16_t eco2,uint16_t tvoc){
  String line = "AQI=" + String(aqi) + " CO2=" + String(eco2) + " VOC=" + String(tvoc);
  if(line != lastLine){
    oled.clear();
    oled.setCursor(0,0);
    oled.print(line);
    lastLine = line;
  }
}

void showPage2(int count,int fan){
  String line = "Count=" + String(count) + " FanSpeed=" + String(fan);
  if(line != lastLine){
    oled.clear();
    oled.setCursor(0,0);
    oled.print(line);
    lastLine = line;
  }
}

// ---------- Zero-padded timestamp helper ----------
void printTimestampPadded(File &f, const DateTime &t) {
  // YYYY-M-D,HH:MM:SS with zero-padded time
  f.print(t.year()); f.print('-'); f.print(t.month()); f.print('-'); f.print(t.day()); f.print(',');
  if (t.hour()   < 10) f.print('0'); f.print(t.hour());   f.print(':');
  if (t.minute() < 10) f.print('0'); f.print(t.minute()); f.print(':');
  if (t.second() < 10) f.print('0'); f.print(t.second());
}

// ---------- Debounce ----------
void updateDebounced(Debounced& s){
  bool n=digitalRead(s.pin); unsigned long t=millis();
  if(n!=s.raw){s.raw=n;s.tchg=t;}
  if((t-s.tchg)>=EDGE_DEBOUNCE_MS) s.stable=s.raw;
}

// ---------- Setup ----------
void setup(){
  Serial.begin(9600); while(!Serial){}
  Wire.begin(); delay(30);

  // RTC (already holds local time if you set via __DATE__/__TIME__)
  if(!rtc.begin()){Serial.println(F("RTC missing"));while(1);}
  if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));

  // OLED
  oled.begin(&Adafruit128x32, OLED_ADDR);
  oled.setFont(Adafruit5x7);
  oled.clear();
  oled.print(F("Initializing..."));

  // ENS160
  if(!ens160.begin()){Serial.println(F("ENS160 missing"));while(1);}
  ens160.setMode(ENS160_OPMODE_STD);

  // SD
  pinMode(SD_CS_PIN,OUTPUT); digitalWrite(SD_CS_PIN,HIGH);
  if(!SD.begin(SD_CS_PIN)){Serial.println(F("SD fail"));while(1);}

  // Fresh file + header
  SD.remove(LOG_FILE);
  {
    File f=SD.open(LOG_FILE,FILE_WRITE);
    if (f) { f.println(F("Date,Time,AQI,eCO2_ppm,TVOC_ppb,PeopleCount,FanSpeed")); f.close(); }
    else { Serial.println(F("Failed to create log file")); }
  }

  // Fan pins
  pinMode(ENA,OUTPUT);
  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);

  // LED pins
  pinMode(LED_GOOD, OUTPUT);
  pinMode(LED_BAD,  OUTPUT);
  digitalWrite(LED_GOOD, LOW);
  digitalWrite(LED_BAD,  LOW);

  // IR sensors
  pinMode(PIN_OUTSIDE,INPUT_PULLUP);
  pinMode(PIN_INSIDE,INPUT_PULLUP);
  sOutside.raw=sOutside.stable=digitalRead(PIN_OUTSIDE);
  sInside.raw=sInside.stable=digitalRead(PIN_INSIDE);
  outsideActivePrev=isActive(sOutside.stable);
  insideActivePrev=isActive(sInside.stable);

  oled.clear();
  oled.print(F("System Ready"));
}

// ---------- Loop ----------
void loop(){
  updateDebounced(sOutside); updateDebounced(sInside);
  bool oA=isActive(sOutside.stable), iA=isActive(sInside.stable);
  bool oR=(oA&&!outsideActivePrev), iR=(iA&&!insideActivePrev);
  unsigned long now=millis();

  switch(seq){
    case IDLE: if(oR){seq=SEEN_OUTSIDE;tSeqStart=now;}
               else if(iR){seq=SEEN_INSIDE;tSeqStart=now;} break;
    case SEEN_OUTSIDE:
      if(iR){peopleCount++;seq=WAIT_BOTH_CLEAR;}
      else if(now-tSeqStart>SEQ_TIMEOUT_MS) seq=WAIT_BOTH_CLEAR; break;
    case SEEN_INSIDE:
      if(oR&&peopleCount>0){peopleCount--;seq=WAIT_BOTH_CLEAR;}
      else if(now-tSeqStart>SEQ_TIMEOUT_MS) seq=WAIT_BOTH_CLEAR; break;
    case WAIT_BOTH_CLEAR:
      if(!oA&&!iA) seq=IDLE; break;
  }
  outsideActivePrev=oA; insideActivePrev=iA;

  // --- sample ENS160 every 10 s ---
  if(millis()-lastSample>=SAMPLE_MS){
    lastSample=millis();
    ens160.measure(true);
    uint8_t aqi=ens160.getAQI();
    uint16_t eco2=ens160.geteCO2();
    uint16_t tvoc=ens160.getTVOC();

    // --- Fan rule ---
    if (aqi <= 1) {
      fanSpeed = 0;
    } else if (aqi == 2) {
      fanSpeed = 70;
    } else if (aqi == 3) {
      fanSpeed = 140;
    } else {
      fanSpeed = 255;
    }
    analogWrite(ENA, fanSpeed);

    // --- LED indication (one LED ON at a time) ---
    if (fanSpeed == 0) {
      digitalWrite(LED_GOOD, HIGH);  // fan off -> LED5 on
      digitalWrite(LED_BAD,  LOW);
    } else {
      digitalWrite(LED_GOOD, LOW);
      digitalWrite(LED_BAD,  HIGH);  // fan on -> LED6 on
    }

    // --- Log to SD with LOCAL time from RTC ---
    DateTime local = rtc.now();   // <- no additional offset
    File f = SD.open(LOG_FILE, FILE_WRITE);
    if (f) {
      printTimestampPadded(f, local); f.print(',');
      f.print(aqi); f.print(','); f.print(eco2); f.print(','); f.print(tvoc); f.print(',');
      f.print(peopleCount); f.print(','); f.println(fanSpeed);
      f.close();
    }

    // Serial preview
    Serial.print(F("Time="));
    if (local.hour() < 10) Serial.print('0'); Serial.print(local.hour()); Serial.print(':');
    if (local.minute() < 10) Serial.print('0'); Serial.print(local.minute()); Serial.print(':');
    if (local.second() < 10) Serial.print('0'); Serial.print(local.second());
    Serial.print(F(" | AQI="));   Serial.print(aqi);
    Serial.print(F(" CO2="));     Serial.print(eco2);
    Serial.print(F(" VOC="));     Serial.print(tvoc);
    Serial.print(F(" Count="));   Serial.print(peopleCount);
    Serial.print(F(" FanSpeed="));Serial.println(fanSpeed);
  }

  // --- Alternate OLED every 3 s ---
  static unsigned long lastPageTime=0;
  static bool page1=true;
  if(millis()-lastPageTime>=3000){
    lastPageTime=millis();
    page1=!page1;
    lastLine = "";  // force refresh on page change
  }

  if(page1)
    showPage1(ens160.getAQI(),ens160.geteCO2(),ens160.getTVOC());
  else
    showPage2(peopleCount,fanSpeed);
}
