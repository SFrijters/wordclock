#define LED_UPDATE_TIME 40
#define LED_TRANSITION_AMOUNT 15
#define LED_RAINBOW_TIME 1000

#define NUM_LEDS 94
#define DATA_PIN D2

#define LDR_PIN         A0
#define LDR_DARK        10
#define LDR_LIGHT       200
#define LDR_READ_DELAY  1000

#ifndef DEBUG_WIFI_LED
#define DEBUG_WIFI_LED 0
#endif
#define WIFI_STATUS_LED 93 // V top right
#define NTP_STATUS_LED 74 // G top right

CRGB leds[NUM_LEDS];
CRGB targetColors[NUM_LEDS];
long lastLedUpdate;

uint8_t ledState = 0;
int8_t ledScrollPosition = 0;
long ledScrollTime = 0;

long ledTestTime = 0;
uint8_t ledTestHue = 0;
int8_t ledTestWord = -1;

long ledRandomHue = 0;
long ledRandomTime = 0;

long ldrReadTime = 0;
int ldrValue = 0;

uint8_t ledMatrix[][10] = {
  { 84,85,86,87,88,89,90,91,92,93 },
  { 83,82,81,80,79,78,77,76,75,74 },
  { 64,65,66,67,68,69,70,71,72,73 },
  { 63,62,61,60,59,58,57,56,55,54 },
  { 44,45,46,47,48,49,50,51,52,53 },
  { 43,42,41,40,39,38,37,36,35,34 },
  { 24,25,26,27,28,29,30,31,32,33 },
  { 23,22,21,20,19,18,17,16,15,14 },
  {  4, 5, 6, 7, 8, 9,10,11,12,13 }
};

uint8_t startupText[][23] = {
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 1,1,1,0,1,0,1,1,0,0,1,0,0,0,0,1,0,0,1,0,0,1,1 },
  { 1,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,1,0,1,0,1,0,0 },
  { 1,1,1,0,1,0,0,1,1,0,0,1,1,0,1,1,1,0,1,0,1,0,0 }
};

void ledSetup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  FastLED.show();

  ledScrollPosition = -sizeof(ledMatrix[0]);
}

void nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount)
{
  if( cur == target) return;

  if( cur < target ) {
    uint8_t delta = target - cur;
    delta = scale8_video( delta, amount);
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = scale8_video( delta, amount);
    cur -= delta;
  }
}

CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount)
{
  nblendU8TowardU8( cur.red,   target.red,   amount);
  nblendU8TowardU8( cur.green, target.green, amount);
  nblendU8TowardU8( cur.blue,  target.blue,  amount);
  return cur;
}

long switchTime;
int state;

byte calculateTimeBrightness() {
    if (hour() > config.brightnessStartHour && hour() < config.brightnessEndHour) {
        return config.maxBrightness;
    } else if (hour() < config.brightnessStartHour || hour() > config.brightnessEndHour) {
        return config.minBrightness;
    } else if (hour() == config.brightnessStartHour) {
        return constrain(
                map(minute(), 0, 29, config.minBrightness, config.maxBrightness),
                config.minBrightness, config.maxBrightness);
    } else if (hour() == config.brightnessEndHour) {
        return constrain(
                map(minute(), 0, 29, config.maxBrightness, config.minBrightness),
                config.minBrightness, config.maxBrightness);
    } else {
        // Unreachable?
        return 0;
    }
}

byte lastBrightness;
byte ledBrightness() {
  byte targetBrightness = config.maxBrightness;
  switch(config.brightnessMode) {
    case fixedBrightness: targetBrightness = config.maxBrightness; break;
    case timeBrightness: targetBrightness = calculateTimeBrightness(); break;
    case ldrBrightness: targetBrightness = constrain(
                map(ldrValue, LDR_DARK, LDR_LIGHT, config.minBrightness, config.maxBrightness),
                config.minBrightness, config.maxBrightness); break;
  }

  if (lastBrightness == 0) {
    lastBrightness = targetBrightness;
  } else {
    int difference = abs(targetBrightness - lastBrightness);
    if (targetBrightness > lastBrightness) {
        lastBrightness = lastBrightness + (1 + 30 * difference / 189 );
    }else if (targetBrightness < lastBrightness) {
        lastBrightness = lastBrightness - (1 + 30 * difference / 189 );
    }
  }

  return lastBrightness;
}

void ledFadeToAssignedColors() {
  if ((millis() - lastLedUpdate) > LED_UPDATE_TIME) {
    lastLedUpdate = millis();

    for(int i = 0; i < NUM_LEDS; i++) {
      fadeTowardColor(leds[i], targetColors[i], LED_TRANSITION_AMOUNT);
    }

    FastLED.show(ledBrightness());
  }
}

void ledScrollIntro() {
  if ((millis() - ledScrollTime) > 250) {
    // Serial.println("ledScrollIntro");
    ledScrollTime = millis();

    for(int y = 0; y < 5; y++) {
      for(int x = 0; x < sizeof(ledMatrix[y]); x++) {
        uint8_t isActive = ((x + ledScrollPosition < sizeof(startupText[y])) && (x + ledScrollPosition >= 0)) ?
          startupText[y][x + ledScrollPosition] : 0;

        CRGB color = isActive ? CRGB(255, 25, 0) : CRGB::Black;
        uint8_t position = ledMatrix[y + 2][x];
        leds[position] = color;
      }
    }
    FastLED.show(ledBrightness());

    int8_t scrollsize = (sizeof(startupText[0]) + sizeof(ledMatrix[0]));
    ledScrollPosition++;
    if (ledScrollPosition > scrollsize) ledState++;
  }
}

void ledShowClockface() {
  // Serial.println("ledShowClockface");
  time_t t = now();

  for(int i = 0; i < NUM_LEDS; i++) {
    targetColors[i] = CRGB::Black;
  }

  CRGB color = CRGB::Black;
  switch(config.ledMode) {
    case single: color.setHue(config.singleColorHue); break;
    case hourly: color.setHue(config.hourlyColors[hour()]); break;
    case rainbow: color.setHue(ledRandomHue); break;
  }

  for(ClockfaceWord word : clockface) {
    if (word.isActive(hour() % 12, minute())) {
      for(int pos : word.leds) {
        if (config.ledMode != words) {
          targetColors[pos] = color;
        } else {
          targetColors[pos].setHue(config.wordColors[word.colorCodeInTable]);
        }
      }
    }
  }
}

void ledShowTestColor() {
  // Serial.println("ledShowTestColor");
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }

  for(ClockfaceWord word : clockface) {
    if (ledTestWord == -1) {
      if (word.isActive(hour() % 12, minute())) {
        for(int pos : word.leds) {
          leds[pos].setHue(ledTestHue);
        }
      }
    } else {
      if (word.colorCodeInTable == ledTestWord) {
        for(int pos : word.leds) {
          leds[pos].setHue(ledTestHue);
        }
      }
    }
  }
  FastLED.show(ledBrightness());
}

void ledSetWifiStatus() {
  const wl_status_t status = WiFi.status();
  if (wifiIsAccessPointActive()) { // purple
    leds[WIFI_STATUS_LED] = CRGB(128, 0, 128);
  }
  else if (status == WL_NO_SHIELD) { // magenta
    leds[WIFI_STATUS_LED] = CRGB(255, 0, 255);
  }
  else if (status == WL_IDLE_STATUS) { // off
    leds[WIFI_STATUS_LED] = CRGB::Black;
  }
  else if (status == WL_NO_SSID_AVAIL) { // blue
    leds[WIFI_STATUS_LED] = CRGB(0, 0, 255);
  }
  else if (status == WL_SCAN_COMPLETED) { // off
    leds[WIFI_STATUS_LED] = CRGB::Black;
  }
  else if (status == WL_CONNECTED) { // off (or green)
#if DEBUG_WIFI_LED
    leds[WIFI_STATUS_LED] = CRGB(0, 255, 0);
#else
    leds[WIFI_STATUS_LED] = CRGB::Black;
#endif
  }
  else if (status == WL_CONNECT_FAILED) { // red
    leds[WIFI_STATUS_LED] = CRGB(255, 0, 0);
  }
  else if (status == WL_CONNECTION_LOST) { // yellow
    leds[WIFI_STATUS_LED] = CRGB(255, 255, 0);
  }
  else if (status == WL_WRONG_PASSWORD) { // blue
    leds[WIFI_STATUS_LED] = CRGB(0, 0, 255);
  }
  else if (status == WL_DISCONNECTED) { // yellow
    leds[WIFI_STATUS_LED] = CRGB(255, 255, 0);
  }
  else {
    leds[WIFI_STATUS_LED] = CRGB(255, 255, 255);
  }
}

void ledSetNtpStatus() {
  const bool validNtpTime = timeStatus() != timeNotSet;
  if (validNtpTime) {
    leds[NTP_STATUS_LED] = CRGB::Black;
  } else { // red
    leds[NTP_STATUS_LED] = CRGB(255, 0, 0);
  }
}

void ledShowNoNTPStatus() {
  if ((millis() - lastLedUpdate) > LED_UPDATE_TIME) {
    // Serial.println("ledShowNoNTPStatus");
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    leds[3] = CRGB::Blue;
    leds[2] = CRGB::Blue;
    FastLED.show(ledBrightness());
  }
}

bool ledHasLDR() {
  return ldrValue < LDR_DARK;
}

void ledLoop() {
  long current = millis();

  if ((current - ldrReadTime) > LDR_READ_DELAY) {
    ldrReadTime = current;
    ldrValue = analogRead(LDR_PIN);
  }

  if ((current - ledRandomTime) > LED_RAINBOW_TIME) {
    ledRandomTime = current;
    ledRandomHue++;
  }

  if (ledState) {
    const bool isLedTestActive = ledTestTime > current;
    if (isLedTestActive) {
      ledShowTestColor();
    } else {
      ledSetWifiStatus();
      ledSetNtpStatus();
      ledShowClockface();
      ledFadeToAssignedColors();
    }
  } else {
    ledScrollIntro();
  }
}
