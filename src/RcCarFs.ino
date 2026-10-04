// ESP8266 LOLIN(WEMOS) D1 R2 & mini

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "wifi_config.h"
#include "debug_log.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/");
DNSServer dnsServer;
unsigned long lastHeapLogMs = 0;

void logHeapDiagnostics(bool force = false) {
  const unsigned long now = millis();
  if (!force && now - lastHeapLogMs < 10000) {
    return;
  }

  lastHeapLogMs = now;
  RC_LOG_INFO("Heap: free=%u B, max block=%u B, fragmentation=%u%%\n",
              (unsigned)ESP.getFreeHeap(),
              (unsigned)ESP.getMaxFreeBlockSize(),
              (unsigned)ESP.getHeapFragmentation());
}

void createAp() {
  RC_LOG_INFO("Configuring access point...\n");
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
    RC_LOG_ERROR("Failed to start access point\n");
    return;
  }

  IPAddress myIP = WiFi.softAPIP();
  RC_LOG_INFO("AP IP address: %s\n", myIP.toString().c_str());

  if (!dnsServer.start(53, "*", myIP)) {
    RC_LOG_ERROR("Failed to start captive portal DNS server\n");
  }
}

void createSta() {
  RC_LOG_INFO("Connecting to Wi-Fi...\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    RC_LOG_DEBUG(".");
  }
  RC_LOG_DEBUG("\n");
  RC_LOG_INFO("Wi-Fi connected, IP address: %s\n", WiFi.localIP().toString().c_str());
}

const int iBlueLedPin = LED_BUILTIN;
const int iLightLedPin = D8;

const int pwmMotorA = D2;
const int in1MotorA = D1;
const int in2MotorA = D3;
const int pwmMotorB = D6;
const int in1MotorB = D5;
const int in2MotorB = D7;

// State
struct State {
  bool isBlueLedOn;
  int leftMotor;
  int rightMotor;
};
State state = { false, 0, 0 };

void setBlueLedOn(bool value) {
  if (state.isBlueLedOn != value) {
    state.isBlueLedOn = value;
    RC_LOG_DEBUG("LED state changed: %d\n", value);
  }
}

void setLeftMotor(int value) {
  if (state.leftMotor != value) {
    state.leftMotor = value;
    RC_LOG_DEBUG("Left motor value changed: %d\n", value);
  }
}

void setRightMotor(int value) {
  if (state.rightMotor != value) {
    state.rightMotor = value;
    RC_LOG_DEBUG("Right motor value changed: %d\n", value);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info == nullptr || !info->final || info->index != 0 ||
      info->len != len || info->opcode != WS_TEXT) {
    return;
  }

  StaticJsonDocument<256> jsonData;
  DeserializationError error = deserializeJson(
      jsonData, reinterpret_cast<const char *>(data), len);
  if (error) {
    RC_LOG_ERROR("Invalid WebSocket JSON: %s\n", error.c_str());
    return;
  }

  if (!jsonData["action"].is<const char*>()) {
    RC_LOG_ERROR("WebSocket JSON is missing a valid action\n");
    return;
  }

  const char* action = jsonData["action"];
  RC_LOG_DEBUG("WebSocket action: %s\n", action);
  if (strcmp(action, "message") == 0) {
    if (!jsonData["value"].is<const char*>()) {
      RC_LOG_ERROR("WebSocket message action requires a string value\n");
      return;
    }
    RC_LOG_DEBUG("WebSocket message: %s\n", jsonData["value"].as<const char*>());
  } else if (strcmp(action, "update") == 0) {
    if (!jsonData["value"].is<JsonObject>()) {
      RC_LOG_ERROR("WebSocket update action requires an object value\n");
      return;
    }

    JsonObject value = jsonData["value"].as<JsonObject>();
    if (value.containsKey("isBlueLedOn") && !value["isBlueLedOn"].is<bool>()) {
      RC_LOG_ERROR("WebSocket update has an invalid isBlueLedOn value\n");
      return;
    }
    if (value.containsKey("leftMotor") &&
        (!value["leftMotor"].is<int>() || value["leftMotor"].as<int>() < -255 ||
         value["leftMotor"].as<int>() > 255)) {
      RC_LOG_ERROR("WebSocket update has an invalid leftMotor value\n");
      return;
    }
    if (value.containsKey("rightMotor") &&
        (!value["rightMotor"].is<int>() || value["rightMotor"].as<int>() < -255 ||
         value["rightMotor"].as<int>() > 255)) {
      RC_LOG_ERROR("WebSocket update has an invalid rightMotor value\n");
      return;
    }

    if (value.containsKey("isBlueLedOn")) {
      setBlueLedOn(value["isBlueLedOn"].as<bool>());
    }
    if (value.containsKey("leftMotor")) {
      setLeftMotor(value["leftMotor"].as<int>());
    }
    if (value.containsKey("rightMotor")) {
      setRightMotor(value["rightMotor"].as<int>());
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  StaticJsonDocument<200> jsonState;
  char stringState[256];
  switch (type) {
    case WS_EVT_CONNECT:
      RC_LOG_DEBUG("WebSocket client #%u connected from %s\n",
                   client->id(), client->remoteIP().toString().c_str());
      jsonState["action"] = "update";
      jsonState["value"]["isBlueLedOn"] = state.isBlueLedOn;
      size_t jsonLength = serializeJson(jsonState, stringState, sizeof(stringState));
      if (jsonLength == 0 || jsonLength >= sizeof(stringState)) {
        RC_LOG_ERROR("Failed to serialize WebSocket state\n");
        break;
      }
      client->text(stringState);
      break;
    case WS_EVT_DISCONNECT:
      RC_LOG_DEBUG("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  while(!Serial);
  RC_LOG_INFO("RC car controller starting\n");

  if (!LittleFS.begin()) {
    RC_LOG_ERROR("Failed to mount LittleFS filesystem\n");
  }
  logHeapDiagnostics(true);

  // prepare LED
  pinMode(iBlueLedPin, OUTPUT);
  // switch LED off
  digitalWrite(iBlueLedPin, HIGH);

  // prepare Lights
  pinMode(iLightLedPin, OUTPUT);

  // prepare Motors
  pinMode(pwmMotorA, OUTPUT);
  pinMode(in1MotorA, OUTPUT);
  pinMode(in2MotorA, OUTPUT);
  pinMode(pwmMotorB, OUTPUT);
  pinMode(in1MotorB, OUTPUT);
  pinMode(in2MotorB, OUTPUT);

#if RC_CAR_WIFI_MODE_AP
  createAp();
#else
  createSta();
#endif

  initWebSocket();

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

#if RC_CAR_WIFI_MODE_AP
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });
#else
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
#endif

  // Start server
  server.begin();
  RC_LOG_INFO("HTTP server started\n");
}

void loop() {
  logHeapDiagnostics();
#if RC_CAR_WIFI_MODE_AP
  dnsServer.processNextRequest();
#endif
  ws.cleanupClients();
  digitalWrite(iLightLedPin, state.isBlueLedOn ? HIGH : LOW);

  // right
  if (state.rightMotor > 70) {
    // forward
    analogWrite(pwmMotorA, state.rightMotor);
    digitalWrite(in1MotorA, HIGH);
    digitalWrite(in2MotorA, LOW);
  } else if (state.rightMotor < -70) {
    // backward
    analogWrite(pwmMotorA, -1 * state.rightMotor);
    digitalWrite(in1MotorA, LOW);
    digitalWrite(in2MotorA, HIGH);
  } else {
    // stop
    analogWrite(pwmMotorA, 0);
    digitalWrite(in1MotorA, LOW);
    digitalWrite(in2MotorA, LOW);
  }

  // left
  if (state.leftMotor > 70) {
    // forward
    analogWrite(pwmMotorB, state.leftMotor);
    digitalWrite(in1MotorB, HIGH);
    digitalWrite(in2MotorB, LOW);
  } else if (state.leftMotor < -70) {
    // backward
    analogWrite(pwmMotorB, -1 * state.leftMotor);
    digitalWrite(in1MotorB, LOW);
    digitalWrite(in2MotorB, HIGH);
  } else {
    // stop
    analogWrite(pwmMotorB, 0);
    digitalWrite(in1MotorB, LOW);
    digitalWrite(in2MotorB, LOW);
  }

}
