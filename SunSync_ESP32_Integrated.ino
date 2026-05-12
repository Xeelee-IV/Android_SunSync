#include <FastLED.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <Preferences.h>
#include <NTPClient.h>
#include <ArduinoJson.h>


// DEVICE CONFIG

#define DATA_PIN 26
#define NUM_LEDS 20
#define CHIPSET SK6812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// SINGLETONS

WiFiManager wifiManager;
WebServer server(80);
Preferences prefs;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


// CYCLE CONFIG STRUCTS

struct CycleConfig {
  CRGB startColor;
  int startBrightness;
  CRGB endColor;
  int endBrightness;
  String startTime;  // "HH:MM"
  int duration;      // minutes
};

CycleConfig sleepConfig;
CycleConfig wakeConfig;
String deviceUUID;
String configUUID;
String apName = "";


// MANUAL COLOR OVERRIDE STATE (existing)

const unsigned long COLOR_OVERRIDE_MS = 2000;
bool colorOverrideActive = false;
unsigned long colorOverrideUntilMs = 0;


// EMERGENCY FLASH STATE (NEW)

struct EmergencyState {
  bool      active          = false;
  String    protocol        = "";
  int       flashIntervalMs = 500;
  uint32_t  startTime       = 0;
  uint32_t  lastToggle      = 0;
  bool      lightsOn        = false;
};

EmergencyState emergencyState;


// HELPER FUNCTIONS

String generateUUID() {
  String uuid = "";
  for(int i = 0; i < 36; i++) {
    if(i == 8 || i == 13 || i == 18 || i == 23) {
      uuid += "-";
    } else if(i == 14) {
      uuid += "4";
    } else if(i == 19) {
      int y = random(8, 12);
      uuid += String(y, HEX);
    } else {
      uuid += String(random(0, 16), HEX);
    }
  }
  return uuid;
}

int timeToSeconds(String time) {
  int colon = time.indexOf(':');
  if (colon == -1) return 0;
  int h = time.substring(0, colon).toInt();
  int m = time.substring(colon + 1).toInt();
  return h * 3600 + m * 60;
}

// LED CONTROL FUNCTIONS

void setColor(const CRGB &color, const int &brightness) {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  fill_solid(leds, NUM_LEDS, color);
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void setAllLightsOff() {
  setColor(CRGB::Black, 0);
}

void setAllLightsMaxBrightness() {
  setColor(CRGB::White, 255);
}

void restoreNormalLighting() {
  colorOverrideActive = false;
  updateColor();
}


// SCHEDULED COLOR UPDATE (respects emergency state)

void updateColor() {
  if (emergencyState.active) {
    return;
  }

  unsigned long currentEpoch = timeClient.getEpochTime();
  unsigned long currentSeconds = currentEpoch % 86400;

  bool inCycle = false;
  CRGB currentColor;
  int currentBrightness;

  // Check sleep cycle
  int sleepStart = timeToSeconds(sleepConfig.startTime);
  int sleepEnd = sleepStart + sleepConfig.duration * 60;
  if (currentSeconds >= sleepStart && currentSeconds < sleepEnd) {
    float progress = (float)(currentSeconds - sleepStart) / (sleepConfig.duration * 60.0);
    currentColor = blend(sleepConfig.startColor, sleepConfig.endColor, progress * 255);
    currentBrightness = sleepConfig.startBrightness + (sleepConfig.endBrightness - sleepConfig.startBrightness) * progress;
    inCycle = true;
  }

  // Check wake cycle
  int wakeStart = timeToSeconds(wakeConfig.startTime);
  int wakeEnd = wakeStart + wakeConfig.duration * 60;
  if (currentSeconds >= wakeStart && currentSeconds < wakeEnd) {
    float progress = (float)(currentSeconds - wakeStart) / (wakeConfig.duration * 60.0);
    currentColor = blend(wakeConfig.startColor, wakeConfig.endColor, progress * 255);
    currentBrightness = wakeConfig.startBrightness + (wakeConfig.endBrightness - wakeConfig.startBrightness) * progress;
    inCycle = true;
  }

  if (inCycle) {
    setColor(currentColor, currentBrightness);
  }
}


// EMERGENCY FLASH HANDLER (NEW)

void handleEmergencyFlash() {
  if (!emergencyState.active) return;

  uint32_t now = millis();

  if (now - emergencyState.lastToggle >= (uint32_t)emergencyState.flashIntervalMs) {
    emergencyState.lightsOn = !emergencyState.lightsOn;
    emergencyState.lastToggle = now;

    if (emergencyState.lightsOn) {
      setAllLightsMaxBrightness();
    } else {
      setAllLightsOff();
    }
  }

  // Auto-cancel after 10 minutes (safety fallback)
  if (now - emergencyState.startTime > 600000UL) {
    Serial.println("[EMERGENCY] Auto-cancelled after 10 min timeout.");
    emergencyState.active = false;
    restoreNormalLighting();
  }
}


// CONFIG PERSISTENCE

void loadConfig() {
  if (prefs.isKey("uuid")) {
    deviceUUID = prefs.getString("uuid");
  } else {
    deviceUUID = "uuid-" + String(random(1000000, 9999999));
    prefs.putString("uuid", deviceUUID);
  }

  if (prefs.isKey("config_uuid")) {
    configUUID = prefs.getString("config_uuid");
  } else {
    configUUID = generateUUID();
    prefs.putString("config_uuid", configUUID);
  }

  // Load sleep config
  sleepConfig.startColor.r = prefs.getInt("sleep_start_r", 255);
  sleepConfig.startColor.g = prefs.getInt("sleep_start_g", 255);
  sleepConfig.startColor.b = prefs.getInt("sleep_start_b", 255);
  sleepConfig.startBrightness = prefs.getInt("sleep_start_bright", 100);
  sleepConfig.endColor.r = prefs.getInt("sleep_end_r", 0);
  sleepConfig.endColor.g = prefs.getInt("sleep_end_g", 0);
  sleepConfig.endColor.b = prefs.getInt("sleep_end_b", 0);
  sleepConfig.endBrightness = prefs.getInt("sleep_end_bright", 0);
  sleepConfig.startTime = prefs.getString("sleep_start_time", "22:00");
  sleepConfig.duration = prefs.getInt("sleep_duration", 60);

  // Load wake config
  wakeConfig.startColor.r = prefs.getInt("wake_start_r", 0);
  wakeConfig.startColor.g = prefs.getInt("wake_start_g", 0);
  wakeConfig.startColor.b = prefs.getInt("wake_start_b", 0);
  wakeConfig.startBrightness = prefs.getInt("wake_start_bright", 0);
  wakeConfig.endColor.r = prefs.getInt("wake_end_r", 255);
  wakeConfig.endColor.g = prefs.getInt("wake_end_g", 255);
  wakeConfig.endColor.b = prefs.getInt("wake_end_b", 255);
  wakeConfig.endBrightness = prefs.getInt("wake_end_bright", 100);
  wakeConfig.startTime = prefs.getString("wake_start_time", "06:00");
  wakeConfig.duration = prefs.getInt("wake_duration", 60);
}

void saveConfig() {
  prefs.putString("config_uuid", configUUID);

  prefs.putInt("sleep_start_r", sleepConfig.startColor.r);
  prefs.putInt("sleep_start_g", sleepConfig.startColor.g);
  prefs.putInt("sleep_start_b", sleepConfig.startColor.b);
  prefs.putInt("sleep_start_bright", sleepConfig.startBrightness);
  prefs.putInt("sleep_end_r", sleepConfig.endColor.r);
  prefs.putInt("sleep_end_g", sleepConfig.endColor.g);
  prefs.putInt("sleep_end_b", sleepConfig.endColor.b);
  prefs.putInt("sleep_end_bright", sleepConfig.endBrightness);
  prefs.putString("sleep_start_time", sleepConfig.startTime);
  prefs.putInt("sleep_duration", sleepConfig.duration);

  prefs.putInt("wake_start_r", wakeConfig.startColor.r);
  prefs.putInt("wake_start_g", wakeConfig.startColor.g);
  prefs.putInt("wake_start_b", wakeConfig.startColor.b);
  prefs.putInt("wake_start_bright", wakeConfig.startBrightness);
  prefs.putInt("wake_end_r", wakeConfig.endColor.r);
  prefs.putInt("wake_end_g", wakeConfig.endColor.g);
  prefs.putInt("wake_end_b", wakeConfig.endColor.b);
  prefs.putInt("wake_end_bright", wakeConfig.endBrightness);
  prefs.putString("wake_start_time", wakeConfig.startTime);
  prefs.putInt("wake_duration", wakeConfig.duration);
}

//-------------------
// HTTP ENDPOINTS

// GET /api/sunsync
void handleGETCheck() {
  server.send(200, "application/json", "{\"id\": \"" + apName + "\", \"uuid\": \"" + deviceUUID + "\"}");
}

// GET /api/color
void handleGETColor() {
  const CRGB color = leds[0];
  server.send(200, "application/json", "{\"r\": " + String(color.r) + ", \"g\": " + String(color.g) + ", \"b\": " + String(color.b) + ", \"a\": " + String(FastLED.getBrightness()) + "}");
}

// POST /api/color
void handlePOSTColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b") && server.hasArg("a")) {
    int r = constrain(server.arg("r").toInt(), 0, 255);
    int g = constrain(server.arg("g").toInt(), 0, 255);
    int b = constrain(server.arg("b").toInt(), 0, 255);
    int a = constrain(server.arg("a").toInt(), 0, 255);

    CRGB newColor = CRGB(r, g, b);
    setColor(newColor, a);

    colorOverrideActive = true;
    colorOverrideUntilMs = millis() + COLOR_OVERRIDE_MS;

    server.send(200, "application/json", "{\"r\": " + String(r) + ", \"g\": " + String(g) + ", \"b\": " + String(b) + ", \"a\": " + String(a) + "}");
  } else {
    server.send(400, "application/json", "{\"error\": \"Missing r, g, b, a parameters\"}");
  }
}

// GET /api/config
void handleGETConfig() {
  DynamicJsonDocument doc(2048);
  doc["_uuid"] = configUUID;
  doc["_sleep"]["start_color"]["_r"] = sleepConfig.startColor.r;
  doc["_sleep"]["start_color"]["_g"] = sleepConfig.startColor.g;
  doc["_sleep"]["start_color"]["_b"] = sleepConfig.startColor.b;
  doc["_sleep"]["start_brightness"] = sleepConfig.startBrightness;
  doc["_sleep"]["end_color"]["_r"] = sleepConfig.endColor.r;
  doc["_sleep"]["end_color"]["_g"] = sleepConfig.endColor.g;
  doc["_sleep"]["end_color"]["_b"] = sleepConfig.endColor.b;
  doc["_sleep"]["end_brightness"] = sleepConfig.endBrightness;
  doc["_sleep"]["start_time"] = sleepConfig.startTime;
  doc["_sleep"]["duration"] = sleepConfig.duration;

  doc["_wake"]["start_color"]["_r"] = wakeConfig.startColor.r;
  doc["_wake"]["start_color"]["_g"] = wakeConfig.startColor.g;
  doc["_wake"]["start_color"]["_b"] = wakeConfig.startColor.b;
  doc["_wake"]["start_brightness"] = wakeConfig.startBrightness;
  doc["_wake"]["end_color"]["_r"] = wakeConfig.endColor.r;
  doc["_wake"]["end_color"]["_g"] = wakeConfig.endColor.g;
  doc["_wake"]["end_color"]["_b"] = wakeConfig.endColor.b;
  doc["_wake"]["end_brightness"] = wakeConfig.endBrightness;
  doc["_wake"]["start_time"] = wakeConfig.startTime;
  doc["_wake"]["duration"] = wakeConfig.duration;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

// POST /api/config
void handlePOSTConfig() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
      return;
    }

    if (doc.containsKey("_uuid")) {
      configUUID = doc["_uuid"].as<String>();
      prefs.putString("config_uuid", configUUID);
    }

    if (doc.containsKey("_sleep")) {
      JsonObject sleep = doc["_sleep"];
      if (sleep.containsKey("start_color")) {
        sleepConfig.startColor.r = sleep["start_color"]["_r"];
        sleepConfig.startColor.g = sleep["start_color"]["_g"];
        sleepConfig.startColor.b = sleep["start_color"]["_b"];
      }
      if (sleep.containsKey("start_brightness")) sleepConfig.startBrightness = sleep["start_brightness"];
      if (sleep.containsKey("end_color")) {
        sleepConfig.endColor.r = sleep["end_color"]["_r"];
        sleepConfig.endColor.g = sleep["end_color"]["_g"];
        sleepConfig.endColor.b = sleep["end_color"]["_b"];
      }
      if (sleep.containsKey("end_brightness")) sleepConfig.endBrightness = sleep["end_brightness"];
      if (sleep.containsKey("start_time")) sleepConfig.startTime = sleep["start_time"].as<String>();
      if (sleep.containsKey("duration")) sleepConfig.duration = sleep["duration"];
    }

    if (doc.containsKey("_wake")) {
      JsonObject wake = doc["_wake"];
      if (wake.containsKey("start_color")) {
        wakeConfig.startColor.r = wake["start_color"]["_r"];
        wakeConfig.startColor.g = wake["start_color"]["_g"];
        wakeConfig.startColor.b = wake["start_color"]["_b"];
      }
      if (wake.containsKey("start_brightness")) wakeConfig.startBrightness = wake["start_brightness"];
      if (wake.containsKey("end_color")) {
        wakeConfig.endColor.r = wake["end_color"]["_r"];
        wakeConfig.endColor.g = wake["end_color"]["_g"];
        wakeConfig.endColor.b = wake["end_color"]["_b"];
      }
      if (wake.containsKey("end_brightness")) wakeConfig.endBrightness = wake["end_brightness"];
      if (wake.containsKey("start_time")) wakeConfig.startTime = wake["start_time"].as<String>();
      if (wake.containsKey("duration")) wakeConfig.duration = wake["duration"];
    }

    saveConfig();
    timeClient.update();
    updateColor();
    server.send(200, "application/json", "{\"status\": \"Config updated\"}");
  } else {
    server.send(400, "application/json", "{\"error\": \"No body\"}");
  }
}


// POST /api/emergency/flash (NEW)

void handlePOSTEmergencyFlash() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
      return;
    }

    const char* protocol        = doc["protocol"]          | "general_hazard";
    const char* title           = doc["title"]             | "Emergency Alert";
    int         flashIntervalMs = doc["flash_interval_ms"] | 500;

    emergencyState.active          = true;
    emergencyState.protocol        = String(protocol);
    emergencyState.flashIntervalMs = flashIntervalMs;
    emergencyState.startTime       = millis();
    emergencyState.lastToggle      = millis();
    emergencyState.lightsOn        = true;

    Serial.printf("[EMERGENCY] Protocol '%s' activated. Interval: %dms\n",
                  protocol, flashIntervalMs);

    DynamicJsonDocument resp(256);
    resp["success"]          = true;
    resp["active_protocol"]  = protocol;
    resp["message"]          = "Emergency flash protocol started";

    String respStr;
    serializeJson(resp, respStr);
    server.send(200, "application/json", respStr);
  } else {
    server.send(400, "application/json", "{\"error\": \"No body\"}");
  }
}



// POST /api/emergency/cancel (NEW)

void handlePOSTEmergencyCancel() {
  emergencyState.active = false;
  restoreNormalLighting();

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Emergency protocol cancelled\"}");

  Serial.println("[EMERGENCY] Protocol cancelled by app.");
}



// STARTUP & SERVER CONFIG

void serverConfigure() {
  server.on("/api/sunsync", HTTP_GET, handleGETCheck);
  server.on("/api/color", HTTP_GET, handleGETColor);
  server.on("/api/color", HTTP_POST, handlePOSTColor);
  server.on("/api/config", HTTP_GET, handleGETConfig);
  server.on("/api/config", HTTP_POST, handlePOSTConfig);
  server.on("/api/emergency/flash", HTTP_POST, handlePOSTEmergencyFlash);
  server.on("/api/emergency/cancel", HTTP_POST, handlePOSTEmergencyCancel);
  server.begin();
}

void rainbowAcrossStrip() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
    FastLED.show();
    leds[i] = CHSV(i * 255 / NUM_LEDS, 255, 255);
    FastLED.show();
    delay(50);
  }
}

void configModeCallback(WiFiManager *myWiFiManager) {
  setColor(CRGB::Blue, 16);
}

void setup() {
  Serial.begin(115200);
  prefs.begin("SunSync");

  if (prefs.isKey("device_id")) {
    apName = prefs.getString("device_id", "SunSync-Unknown");
  } else {
    String randomSuffix = String(random(1000, 9999));
    apName = "SunSync-" + randomSuffix;
    prefs.putString("device_id", apName);
  }

  loadConfig();

  delay(1500);
  Serial.println("\n\n");

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(16);
  rainbowAcrossStrip();

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect(apName.c_str());

  if (!MDNS.begin(apName)) {
    Serial.println("Error setting up MDNS responder!");
  }
  MDNS.addService("http", "tcp", 80);

  timeClient.begin();

  setColor(CRGB::Green, 16);
  delay(800);

  setColor(CRGB::Black, 0);

  serverConfigure();

  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  timeClient.update();
  server.handleClient();

  // Handle manual color override
  if (colorOverrideActive) {
    unsigned long nowMs = millis();
    if ((long)(nowMs - colorOverrideUntilMs) >= 0) {
      colorOverrideActive = false;
      updateColor();
    }
  } else {
    updateColor();
  }

  // Handle emergency flashing
  handleEmergencyFlash();
}
