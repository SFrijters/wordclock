// Inspired by https://github.com/niekproductions/word-clock

// Required Libraries:
// - Arduino JSON (https://arduinojson.org/)
// - Time library (http://playground.arduino.cc/code/time)

// Web portal requires files on SPIFFS, please see the following URL for
// more information:

// http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html

#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <TimeLib.h>
#include <vector>
#include <ESP8266WiFi.h>
#include "ESP8266HTTPUpdateServer.h"
#include "ArduinoJson.h"

const char *version = "0.1";

#define MAX_WIFI_NETWORKS 32
#define MAX_SSID_LENGTH 32
#define WIFI_NETWORK_UPDATE_INTERVAL 60000
#define HOSTNAME_MAX 256

struct WifiNetwork {
  char ssid[MAX_SSID_LENGTH + 1];
  uint8_t encryptionType;
  int32_t RSSI;
  int32_t channel;
  bool isHidden;
};

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define FOREACH_LEDMODE(FN) \
        FN(single)          \
        FN(words)           \
        FN(hourly)          \
        FN(rainbow)

typedef enum {
  FOREACH_LEDMODE(GENERATE_ENUM)
} LedMode;

static const char *ledModeStr[] = {
  FOREACH_LEDMODE(GENERATE_STRING)
};

#define FOREACH_BRIGHTNESSMODE(FN) \
        FN(fixedBrightness)        \
        FN(ldrBrightness)          \
        FN(timeBrightness)

typedef enum {
  FOREACH_BRIGHTNESSMODE(GENERATE_ENUM)
} BrightnessMode;

static const char *brightnessModeStr[] = {
  FOREACH_BRIGHTNESSMODE(GENERATE_STRING)
};

struct Configuration {
  char ntp_server[256];
  char hostname[HOSTNAME_MAX];
  LedMode ledMode;
  uint8_t singleColorHue;
  uint8_t hourlyColors[24];
  uint8_t wordColors[25];
  BrightnessMode brightnessMode;
  uint8_t maxBrightness;
  uint8_t minBrightness;
  uint8_t brightnessStartHour;
  uint8_t brightnessEndHour;
  char ssid[MAX_SSID_LENGTH + 1];
  char password[256];
  uint8_t checksum;
};

struct ClockfaceWord {
  std::vector<int> leds;
  int colorCodeInTable;
  bool (*isActive)(int, int);
};

Configuration config;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10000);                       // wait for ten seconds so we can get the monitor hooked up
  Serial.print("BITLAIR Wordclock, v");
  Serial.println(version);
  Serial.print("ESP ID: ");
  Serial.println(String(ESP.getChipId()));

  Serial.println("configurationSetup");
  configurationSetup();
  Serial.println("wifiSetup");
  wifiSetup();
  Serial.println("timeSetup");
  timeSetup();
  Serial.println("webserverSetup");
  webserverSetup();
  Serial.println("ledSetup");
  ledSetup();
  Serial.println("Setup complete");
}

void loop() {
  wifiLoop();
  timeLoop();
  webserverLoop();
  ledLoop();
}
