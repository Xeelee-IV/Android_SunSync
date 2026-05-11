/*
 * SmartLamp ESP32 Firmware — Emergency Flash API Handler
 * -------------------------------------------------------
 * Add this to your existing ESP32 web server setup.
 *
 * Requires: ESPAsyncWebServer (or Arduino WebServer)
 *           ArduinoJson
 *
 * Android app POSTs to:
 *   POST http://smartlamp.local/api/emergency/flash
 *   POST http://smartlamp.local/api/emergency/cancel
 */

#include <ArduinoJson.h>

// ─── Flash protocol state ────────────────────────────────────────────────────

struct EmergencyState {
  bool      active          = false;
  String    protocol        = "";
  int       flashIntervalMs = 500;
  uint32_t  startTime       = 0;
  uint32_t  lastToggle      = 0;
  bool      lightsOn        = false;
};

EmergencyState emergencyState;

// ─── Register routes (call this in your setup() alongside other routes) ──────

void registerEmergencyRoutes(AsyncWebServer &server) {

  // POST /api/emergency/flash
  server.on("/api/emergency/flash", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

      StaticJsonDocument<512> doc;
      DeserializationError err = deserializeJson(doc, data, len);

      if (err) {
        request->send(400, "application/json",
          "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
      }

      // Parse request fields sent by Android app
      const char* protocol        = doc["protocol"]          | "general_hazard";
      const char* title           = doc["title"]             | "Emergency Alert";
      int         flashIntervalMs = doc["flash_interval_ms"] | 500;

      // Activate the emergency state
      emergencyState.active          = true;
      emergencyState.protocol        = String(protocol);
      emergencyState.flashIntervalMs = flashIntervalMs;
      emergencyState.startTime       = millis();
      emergencyState.lastToggle      = millis();
      emergencyState.lightsOn        = true;

      Serial.printf("[EMERGENCY] Protocol '%s' activated. Interval: %dms\n",
                    protocol, flashIntervalMs);

      // Respond immediately — flashing happens in loop()
      StaticJsonDocument<256> resp;
      resp["success"]          = true;
      resp["active_protocol"]  = protocol;
      resp["message"]          = "Emergency flash protocol started";

      String respStr;
      serializeJson(resp, respStr);
      request->send(200, "application/json", respStr);
    }
  );

  // POST /api/emergency/cancel
  server.on("/api/emergency/cancel", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      emergencyState.active = false;
      restoreNormalLighting();   // your existing function to restore lamp state

      request->send(200, "application/json",
        "{\"success\":true,\"message\":\"Emergency protocol cancelled\"}");

      Serial.println("[EMERGENCY] Protocol cancelled by app.");
    }
  );
}

// ─── Call this from your main loop() ─────────────────────────────────────────

void handleEmergencyFlash() {
  if (!emergencyState.active) return;

  uint32_t now = millis();

  // Toggle lights at the specified interval
  if (now - emergencyState.lastToggle >= (uint32_t)emergencyState.flashIntervalMs) {
    emergencyState.lightsOn  = !emergencyState.lightsOn;
    emergencyState.lastToggle = now;

    if (emergencyState.lightsOn) {
      setAllLightsMaxBrightness();  // your existing LED control function
    } else {
      setAllLightsOff();            // your existing LED control function
    }
  }

  // Optional: auto-cancel after 10 minutes (safety fallback)
  if (now - emergencyState.startTime > 600000UL) {
    Serial.println("[EMERGENCY] Auto-cancelled after 10 min timeout.");
    emergencyState.active = false;
    restoreNormalLighting();
  }
}

/*
 * In your loop():
 *
 *   void loop() {
 *     // ... your existing code ...
 *     handleEmergencyFlash();    // <-- add this
 *   }
 *
 * In your setup():
 *
 *   registerEmergencyRoutes(server);  // <-- add this alongside your other routes
 */
