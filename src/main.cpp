#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <color.h>
#define DEBUG

#define COLORDATA 40

#define LED_PIN     23
#define NUM_LEDS    80
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 100

#define DEGRADINTERVAL 400

#define TRIG_PIN 15

#define LIM NUM_LEDS/3

#define gmtOffset_sec -10800

const char *ssid = "";
const char *pass = "";

int blink[LIM];

uint16_t tNow;
uint64_t last;

uint64_t lastSounds = 0;
uint64_t lastTdegrad;

int brightness = 255;

unsigned long soundDebounce;
bool block = false;
/*int colors[COLORDATA][4] = {
            {0,255,0,0,0},
            {100,255,0,0,0},
            {200,255,0,0,16},
            {300,255,0,0,64},
            {315,255,100,0,128},
            {360,255,226,13,160},
            {380,255,229,38,160},
            {390,255,233,66,160},
            {400,255,238,116,160},
            {410,255,247,195,160},
            {420,254,248,204,160},
            {450,228,248,219,160},
            {480,211,249,234,160},
            {510,209,249,236,160},
            {540,197,250,247,160},
            {570,190,250,254,160},
            {600,171,248,255,160},
            {630,100,241,255,160},
            {660,80,224,255,160},
            {690,40,220,255,160},
            {720,40,210,255,160},
            {750,40,220,255,160},
            {780,80,224,255,160},
            {810,100,241,255,160},
            {840,171,248,255,160},
            {870,190,250,254,160},
            {900,197,250,247,160},
            {930,209,249,236,160},
            {960,211,249,234,160},
            {990,228,248,219,160},
            {1020,254,248,204,160},
            {1050,255,247,195,160},
            {1080,255,238,116,160},
            {1120,255,233,66,160},
            {1150,255,229,38,128},
            {1180,255,226,13,64},
            {1220,255,100,0,16},
            {1250,255,0,0,4},
            {1300,255,0,0,0},
            {1440,255,0,0,0}};*/

CRGB leds[NUM_LEDS];
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", gmtOffset_sec, 60000);

TaskHandle_t Task1;

int ls;
int h, s, v;

void fillLedRGB(int R, int G, int B, int A) {
  FastLED.setBrightness(A);
  for(int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(R, G, B);
}

void fillLedHSV(int H, int S, int V) {
  for(int i = 0; i < NUM_LEDS; i++)
    leds[i] = CHSV(H, S, V);
}

void linearInterpolate(double x) {
  double x0, x1, y0r, y1r, y0g, y1g, y0b, y1b, y0a, y1a;
  int r, g, b, a;
  for (int i = 0; i < COLORDATA-1; i++) {
    if (x > colors[i][0] && x < colors[i + 1][0]) {
        y0r = colors[i][1];
        y1r = colors[i + 1][1];
        y0g = colors[i][2];
        y1g = colors[i + 1][2];
        y0b = colors[i][3];
        y1b = colors[i + 1][3];
        y0a = colors[i][4];
        y1a = colors[i + 1][4];
        x0 = colors[i][0];
        x1 = colors[i + 1][0];
        r = y0r + ((x-x0)*(y1r-y0r)/(x1-x0));
        g = y0g + ((x-x0)*(y1g-y0g)/(x1-x0));
        b = y0b + ((x-x0)*(y1b-y0b)/(x1-x0));
        a = y0a + ((x-x0)*(y1a-y0a)/(x1-x0));
    }
    else if (x == colors[i][0]) {
      r = colors[i][1];
      g = colors[i][2];
      b = colors[i][3];
      a = colors[i][4];
    }
  }
  fillLedRGB(r,g,b,a);
}

void time2color(int t) {
  h = colors[t][0];
  s = colors[t][1];
  v = colors[t][2];
  //Serial.println(String(t)+ " "+String(h)+ " "+String(s)+ " "+String(v));
  fillLedHSV(h, s, v);
}

void startWifi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ArduinoOTA.begin();
  ArduinoOTA.setHostname("ledstrip");
}

void startLed () {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

uint32_t getTime() {
  timeClient.update();
  return timeClient.getHours()*60+timeClient.getMinutes();
}

void updateTime() {
  if(millis() - last > 60000) {
    tNow = getTime();
    last = millis();
  }
}

void IRAM_ATTR soudDetector() {
  if (millis() - soundDebounce > 400 ) {
    if(lastSounds < LIM) lastSounds++;
    soundDebounce = millis();
  }

}

void histSoundDegrad() {
  if(millis() - lastTdegrad > DEGRADINTERVAL) {
    lastTdegrad = millis();
    if(lastSounds > 0) lastSounds--;
  }
}

void startSoundDetection() {
  pinMode(TRIG_PIN, INPUT);
  attachInterrupt(TRIG_PIN, soudDetector, RISING);
}


static void twinkle(void* pvParameters) {
  while(true) {
    ls = lastSounds;
    for (int i = 0; i < ls; i++)  {
      blink[i] = random(0,NUM_LEDS-1);
      //Serial.print(String(blink[i]) + " ");
    }
    //Serial.println();
    block = true;
    for(int i = v; i < 255; i+=5){
      for( int j = 0 ; j < ls ; j++ ) leds[blink[j]] = CHSV(h, s, i);
      FastLED.show();
    }
    FastLED.delay(100 / UPDATES_PER_SECOND);
    block = false;

  }
}

void debug() {
  for(double i = 0; i < 1441; i +=1) {
    time2color(i);
    histSoundDegrad();
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}

void realtime() {
  updateTime();
  time2color(tNow);
  histSoundDegrad();
  Serial.println(block);
  if(!block) {
    Serial.println("t= "+String(tNow));
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}

void setup() {
  Serial.begin(115200);
  startWifi();
  startLed();
  startSoundDetection();
  timeClient.begin();
  tNow = getTime();
  last = lastTdegrad = soundDebounce = millis();
  xTaskCreate(twinkle, "piscar led", 10000, NULL, 0, &Task1);
}

void loop() {
  ArduinoOTA.handle();
  //Serial.println(lastSounds);
  realtime();
  //debug();
  vTaskDelay(pdMS_TO_TICKS(50));
}
