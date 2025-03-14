#include <EEPROM.h>

void configurationSetup() {
  Serial.print("Configuration consists of ");
  Serial.print(sizeof(Configuration));
  Serial.println(" bytes");
  EEPROM.begin(sizeof(Configuration));
  loadConfiguration();
}

void saveConfiguration() {
  Serial.println("Need to save configuration to EEPROM");
  config.checksum = calculateConfigChecksum();
  printConfiguration();
  EEPROM.put(0, config);
  EEPROM.commit();
}

void loadConfiguration() {
  Serial.println("Loading configuration from EEPROM");
  EEPROM.get(0, config);

  uint8_t expected_checksum = calculateConfigChecksum();
  if (config.checksum != expected_checksum) {
    Serial.print("EEPROM checksum not valid, got ");
    Serial.print(config.checksum);
    Serial.print(", expected ");
    Serial.println(expected_checksum);
    loadDefaultConfiguration();
  }
  printConfiguration();
}

void printConfiguration() {
  Serial.println("=== Configuration ===");
  Serial.print("ntp_server: ");
  Serial.println(config.ntp_server);
  Serial.print("hostname: ");
  Serial.println(config.hostname);
  Serial.print("ledMode: ");
  Serial.print(config.ledMode);
  Serial.print(" = ");
  Serial.println(ledModeStr[config.ledMode]);
  Serial.print("singleColorHue: ");
  Serial.println(config.singleColorHue);
  Serial.println("hourlyColors");
  for (int i = 0; i < sizeof(config.hourlyColors); ++i) { Serial.print(" "); Serial.print(config.hourlyColors[i]); }
  Serial.println("");
  Serial.println("wordColors");
  for (int i = 0; i < sizeof(config.wordColors); ++i) { Serial.print(" "); Serial.print(config.wordColors[i]); }
  Serial.println("");
  Serial.print("brightnessMode: ");
  Serial.print(config.brightnessMode);
  Serial.print(" = ");
  Serial.println(brightnessModeStr[config.brightnessMode]);
  Serial.print("minBrightness: ");
  Serial.println(config.minBrightness);
  Serial.print("maxBrightness: ");
  Serial.println(config.maxBrightness);
  Serial.print("brightnessStartHour: ");
  Serial.println(config.brightnessStartHour);
  Serial.print("brightnessEndHour: ");
  Serial.println(config.brightnessEndHour);
  Serial.print("ssid: ");
  Serial.println(config.ssid);
  Serial.print("password: ");
#ifdef DEBUG_PASSWORD
  Serial.println(config.password);
#else
  Serial.println("<redacted>");
#endif
  Serial.print("checksum: ");
  Serial.println(config.checksum);
}

void loadDefaultConfiguration() {
  Serial.println("Loading default configuration");

  char ntpServer[] = "nl.pool.ntp.org";
  strncpy(config.ntp_server, ntpServer, sizeof(ntpServer));

  char hostname[] = "woordklok";
  strncpy(config.hostname, hostname, sizeof(hostname));

  config.ledMode = single;
  config.singleColorHue = 13;
  for(int i = 0; i < sizeof(config.hourlyColors); i++) { config.hourlyColors[i] = random(255); }
  for(int i = 0; i < sizeof(config.wordColors); i++) { config.wordColors[i] = random(255); }

  config.brightnessMode = ldrBrightness;
  config.maxBrightness = 255;
  config.minBrightness = 65;
  config.brightnessStartHour = 8;
  config.brightnessEndHour = 22;
  memset(config.ssid, 0, sizeof(config.ssid));
  memset(config.password, 0, sizeof(config.password));
  config.checksum = calculateConfigChecksum();
}

uint8_t calculateConfigChecksum() {
  uint8_t previousChecksum = config.checksum;
  config.checksum = 0;

  unsigned char *data = (unsigned char *)&config;
  uint8_t checksum = sizeof(Configuration);
  for(int i = 0; i < sizeof(Configuration); i++) {
    checksum ^= data[i];
  }

  config.checksum = previousChecksum;
  return checksum;
}
