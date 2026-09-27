#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

constexpr uint8_t LED_PIN = 2;

constexpr uint8_t RELAY_PIN = 26;
constexpr bool RELAY_ACTIVE_LOW = false;

constexpr uint8_t RGB_R_PIN = 25;
constexpr uint8_t RGB_G_PIN = 33;
constexpr uint8_t RGB_B_PIN = 32;
constexpr bool RGB_COMMON_ANODE = false;

WebServer server(80);
bool ledIsOn = false;
bool relayIsOn = false;
bool rgbR = false;
bool rgbG = false;
bool rgbB = false;

int levelFor(bool on, bool activeLow) {
  if (activeLow) return on ? LOW : HIGH;
  return on ? HIGH : LOW;
}

void applyRelay(bool on) { digitalWrite(RELAY_PIN, levelFor(on, RELAY_ACTIVE_LOW)); }

void applyRgb() {
  digitalWrite(RGB_R_PIN, levelFor(rgbR, RGB_COMMON_ANODE));
  digitalWrite(RGB_G_PIN, levelFor(rgbG, RGB_COMMON_ANODE));
  digitalWrite(RGB_B_PIN, levelFor(rgbB, RGB_COMMON_ANODE));
}

String onOffThai(bool on) { return on ? "เปิดอยู่" : "ปิดอยู่"; }

void sendPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 LED + Relay + RGB</title>
  <style>
    body { font-family: sans-serif; text-align: center; margin: 2rem 1rem; }
    button { padding: 0.6rem 1.2rem; margin: 0.3rem; font-size: 1rem; cursor: pointer; }
    .card { border: 1px solid #ccc; border-radius: 12px; padding: 1rem; margin: 1rem auto; max-width: 480px; }
  </style>
</head>
<body>
  <h1>ESP32 Control</h1>

  <div class="card">
    <h2>LED บนบอร์ด</h2>
    <p>สถานะ: <strong>LED_STATUS</strong></p>
    <a href="/on"><button>เปิด LED</button></a>
    <a href="/off"><button>ปิด LED</button></a>
  </div>

  <div class="card">
    <h2>Relay</h2>
    <p>สถานะ: <strong>RELAY_STATUS</strong></p>
    <a href="/relay/on"><button>เปิด Relay</button></a>
    <a href="/relay/off"><button>ปิด Relay</button></a>
  </div>

  <div class="card">
    <h2>RGB</h2>
    <p>R: <strong>RGB_R</strong> | G: <strong>RGB_G</strong> | B: <strong>RGB_B</strong></p>
    <div>
      <a href="/rgb/r/on"><button>R on</button></a>
      <a href="/rgb/r/off"><button>R off</button></a>
    </div>
    <div>
      <a href="/rgb/g/on"><button>G on</button></a>
      <a href="/rgb/g/off"><button>G off</button></a>
    </div>
    <div>
      <a href="/rgb/b/on"><button>B on</button></a>
      <a href="/rgb/b/off"><button>B off</button></a>
    </div>
    <div>
      <a href="/rgb/all/on"><button>เปิดทุกสี</button></a>
      <a href="/rgb/all/off"><button>ปิดทุกสี</button></a>
    </div>
  </div>
</body>
</html>
)rawliteral";

  page.replace("LED_STATUS", onOffThai(ledIsOn));
  page.replace("RELAY_STATUS", onOffThai(relayIsOn));
  page.replace("RGB_R", onOffThai(rgbR));
  page.replace("RGB_G", onOffThai(rgbG));
  page.replace("RGB_B", onOffThai(rgbB));
  server.send(200, "text/html; charset=utf-8", page);
}

void logChange(const char *name, bool changed, bool on) {
  IPAddress clientIp = server.client().remoteIP();
  Serial.printf("%s %s %s (request from %s)\n", name,
                changed ? "changed to" : "already",
                on ? "ON" : "OFF", clientIp.toString().c_str());
}

void setLed(bool turnOn) {
  bool changed = (ledIsOn != turnOn);
  ledIsOn = turnOn;
  digitalWrite(LED_PIN, ledIsOn ? HIGH : LOW);
  logChange("LED", changed, ledIsOn);
  sendPage();
}

void setRelay(bool turnOn) {
  bool changed = (relayIsOn != turnOn);
  relayIsOn = turnOn;
  applyRelay(relayIsOn);
  logChange("Relay", changed, relayIsOn);
  sendPage();
}

void setRgb(bool r, bool g, bool b, const char *name) {
  rgbR = r;
  rgbG = g;
  rgbB = b;
  applyRgb();
  IPAddress clientIp = server.client().remoteIP();
  Serial.printf("%s -> R:%s G:%s B:%s (request from %s)\n", name,
                rgbR ? "ON" : "OFF", rgbG ? "ON" : "OFF",
                rgbB ? "ON" : "OFF", clientIp.toString().c_str());
  sendPage();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  applyRelay(false);
  applyRgb();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println();
  Serial.print("Connected! Open http://");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, sendPage);
  server.on("/on", HTTP_GET, []() { setLed(true); });
  server.on("/off", HTTP_GET, []() { setLed(false); });

  server.on("/relay/on", HTTP_GET, []() { setRelay(true); });
  server.on("/relay/off", HTTP_GET, []() { setRelay(false); });

  server.on("/rgb/r/on", HTTP_GET, []() { setRgb(true, rgbG, rgbB, "RGB R on"); });
  server.on("/rgb/r/off", HTTP_GET, []() { setRgb(false, rgbG, rgbB, "RGB R off"); });
  server.on("/rgb/g/on", HTTP_GET, []() { setRgb(rgbR, true, rgbB, "RGB G on"); });
  server.on("/rgb/g/off", HTTP_GET, []() { setRgb(rgbR, false, rgbB, "RGB G off"); });
  server.on("/rgb/b/on", HTTP_GET, []() { setRgb(rgbR, rgbG, true, "RGB B on"); });
  server.on("/rgb/b/off", HTTP_GET, []() { setRgb(rgbR, rgbG, false, "RGB B off"); });
  server.on("/rgb/all/on", HTTP_GET, []() { setRgb(true, true, true, "RGB all on"); });
  server.on("/rgb/all/off", HTTP_GET, []() { setRgb(false, false, false, "RGB all off"); });

  server.onNotFound([]() {
    server.send(404, "text/plain; charset=utf-8", "Not Found");
  });
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
