// ESP8266 LOLIN(WEMOS) D1 R2 & mini

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "index.html.h"
#include "nipplejs.js.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/");

void createAp() {
  /* Go to http://192.168.4.1 in a web browser
  */
  const char *ssid = "ESPap";
  const char *password = "thereisnospoon";

  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void createSta() {
  const char *ssid = "honeypot";
  const char *password = "ak5jsl39fjgnr9";

  Serial.print("Configuring access point...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(WiFi.localIP());
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
  // Serial.printf("WebSocketClient setBlueLedOn value: %d\n", value);
  // Serial.printf("WebSocketClient state.isBlueLedOn: %d\n", state.isBlueLedOn);
  if (state.isBlueLedOn != value) {
    state.isBlueLedOn = value;
    // StaticJsonDocument<200> jsonObject;
    // char jsonString[256];
    // jsonObject["action"] = "update";
    // jsonObject["value"]["isBlueLedOn"] = state.isBlueLedOn;
    // size_t len = serializeJson(jsonObject, jsonString);
    // Serial.printf("Sending data: %s, len %d\n", jsonString, len);
    // notify clients
    //ws.textAll(jsonString, len);
  }
}

void setLeftMotor(int value) {
  // Serial.printf("WebSocketClient setLeftMotor value: %d\n", value);
  // Serial.printf("WebSocketClient state.leftMotor: %d\n", state.leftMotor);
  if (state.leftMotor != value) {
    state.leftMotor = value;
    // StaticJsonDocument<200> jsonObject;
    // char jsonString[256];
    // jsonObject["action"] = "update";
    // jsonObject["value"]["leftMotor"] = state.leftMotor;
    // size_t len = serializeJson(jsonObject, jsonString);
    // Serial.printf("Sending data: %s, len %d\n", jsonString, len);
    // notify clients
    // ws.textAll(jsonString, len);
  }
}

void setRightMotor(int value) {
  // Serial.printf("WebSocketClient setRightMotor value: %d\n", value);
  // Serial.printf("WebSocketClient state.rightMotor: %d\n", state.rightMotor);
  if (state.rightMotor != value) {
    state.rightMotor = value;
    // StaticJsonDocument<200> jsonObject;
    // char jsonString[256];
    // jsonObject["action"] = "update";
    // jsonObject["value"]["rightMotor"] = state.rightMotor;
    // size_t len = serializeJson(jsonObject, jsonString);
    // Serial.printf("Sending data: %s, len %d\n", jsonString, len);
    // notify clients
    // ws.textAll(jsonString, len);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    StaticJsonDocument<256> jsonData;
    deserializeJson(jsonData, (char *)data, len);
    
    const char* action = jsonData["action"];
    Serial.printf("handleWebSocketMessage action %s\n", action);
    if (strcmp(action, "message") == 0) {
      const char* value = jsonData["value"];
      Serial.printf("WebSocketClient message %s\n", value);
    } else if (strcmp(action, "update") == 0) {
      if (jsonData["value"].containsKey("isBlueLedOn")) {
        bool isBlueLedOn = jsonData["value"]["isBlueLedOn"];
        // Serial.printf("WebSocketClient value.isBlueLedOn %d\n", isBlueLedOn);
        setBlueLedOn(isBlueLedOn);
      }
      if (jsonData["value"].containsKey("leftMotor")) {
        int leftMotor = jsonData["value"]["leftMotor"];
        // Serial.printf("WebSocketClient value.leftMotor %d\n", leftMotor);
        setLeftMotor(leftMotor);
      }
      if (jsonData["value"].containsKey("rightMotor")) {
        int rightMotor = jsonData["value"]["rightMotor"];
        // Serial.printf("WebSocketClient value.rightMotor %d\n", rightMotor);
        setRightMotor(rightMotor);
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  StaticJsonDocument<200> jsonState;
  char stringState[256];
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      jsonState["action"] = "update";
      jsonState["value"]["isBlueLedOn"] = state.isBlueLedOn;
      serializeJson(jsonState, stringState);
      client->text(stringState);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
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
  Serial.println("Test");

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

  createAp();
  // createSta();

  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/nipplejs.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "application/javascript", nipplejs_js);
  });

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap(), DEC));
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
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
