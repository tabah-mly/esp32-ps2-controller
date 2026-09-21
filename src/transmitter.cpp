#include <Arduino.h>
#include <PS2X_lib.h>
#include <WiFi.h>
#include <esp_now.h>

#define ENABLE_WEB_SERVER 0

#if ENABLE_WEB_SERVER
#include <WebServer.h>
#endif

#define PS2_DAT 23
#define PS2_CMD 19
#define PS2_SEL 5
#define PS2_CLK 18

#define BTN_UP (1 << 0)
#define BTN_RIGHT (1 << 1)
#define BTN_DOWN (1 << 2)
#define BTN_LEFT (1 << 3)

#define BTN_TRIANGLE (1 << 4)
#define BTN_CIRCLE (1 << 5)
#define BTN_CROSS (1 << 6)
#define BTN_SQUARE (1 << 7)

#define BTN_L1 (1 << 8)
#define BTN_L2 (1 << 9)
#define BTN_L3 (1 << 10)

#define BTN_R1 (1 << 11)
#define BTN_R2 (1 << 12)
#define BTN_R3 (1 << 13)

#define BTN_SELECT (1 << 14)
#define BTN_START (1 << 15)

uint8_t receiverMac[] = {0x80, 0xF3, 0xDA, 0x5D, 0x65, 0x68};

#if ENABLE_WEB_SERVER
const char* ssid = "Controller";
const char* password = "12345678";
#endif

const int WIFI_CHANNEL = 1;

struct ControllerFrame {
  int8_t lx;
  int8_t ly;
  int8_t rx;
  int8_t ry;

  uint16_t buttons;
};

struct ButtonMap {
  uint16_t ps2Button;
  uint16_t mask;
};

PS2X ps2x;
#if ENABLE_WEB_SERVER
WebServer server(80);
#endif

int lxCenter = -1;
int lyCenter = -1;
int rxCenter = -1;
int ryCenter = -1;

ControllerFrame latestFrame;
bool lastSendSuccess = false;
unsigned long packetsSent = 0;

int getAverage(byte button, uint16_t avgNum = 100) {
  int sum = 0;
  for (int i = 0; i < avgNum; i++) {
    sum += ps2x.Analog(button);
    delay(10);
  }
  return sum / avgNum;
}

int scaleJoystick(int value, int minValue, int maxValue) {
  if (value < 0) {
    return round(value * 100.0f / abs(minValue));
  } else {
    return round(value * 100.0f / maxValue);
  }
}

ControllerFrame readController() {
  ControllerFrame frame;

  ps2x.read_gamepad(false, false);

  // Joysticks
  frame.lx = scaleJoystick(ps2x.Analog(PSS_LX) - lxCenter, -132, 123);
  frame.ly = scaleJoystick(ps2x.Analog(PSS_LY) - lyCenter, -123, 132);
  frame.rx = scaleJoystick(ps2x.Analog(PSS_RX) - rxCenter, -132, 123);
  frame.ry = scaleJoystick(ps2x.Analog(PSS_RY) - ryCenter, -132, 123);

  // Buttons
  const ButtonMap buttons[] = {
    {PSB_PAD_UP, BTN_UP},
    {PSB_PAD_RIGHT, BTN_RIGHT},
    {PSB_PAD_DOWN, BTN_DOWN},
    {PSB_PAD_LEFT, BTN_LEFT},
    {PSB_TRIANGLE, BTN_TRIANGLE},
    {PSB_CIRCLE, BTN_CIRCLE},
    {PSB_CROSS, BTN_CROSS},
    {PSB_SQUARE, BTN_SQUARE},
    {PSB_L1, BTN_L1},
    {PSB_L2, BTN_L2},
    {PSB_L3, BTN_L3},
    {PSB_R1, BTN_R1},
    {PSB_R2, BTN_R2},
    {PSB_R3, BTN_R3},
    {PSB_SELECT, BTN_SELECT},
    {PSB_START, BTN_START},
  };

  frame.buttons = 0;

  for (const auto& button : buttons) {
    if (ps2x.Button(button.ps2Button)) {
      frame.buttons |= button.mask;
    }
  }

  return frame;
}

void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    lastSendSuccess = true;
    packetsSent++;
  } else {
    lastSendSuccess = false;
  }
}

#if ENABLE_WEB_SERVER
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Controller</title>
  <style>
    body { font-family: Arial, sans-serif; background: #111; color: white; text-align: center; margin: 0; padding: 20px }
    h1 { margin-bottom: 25px; }
    .container { max-width: 500px; margin: auto; }
    .card { background: #222; border-radius: 12px; padding: 20px; margin-bottom: 15px; }
    .value { font-size: 32px; font-weight: bold; }
    .label { color: #aaa; font-size: 14px; }
    .joysticks { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .status { font-size: 20px; }
    .success { color: #00ff88; }
    .failed { color: #ff4444; }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Controller</h1>
    <div class="card">
      <h2>Joysticks</h2>
      <div class="joysticks">
        <div><div class="label">LX</div><div class="value" id="lx">0</div></div>
        <div><div class="label">LY</div><div class="value" id="ly">0</div></div>
        <div><div class="label">RX</div><div class="value" id="rx">0</div></div>
        <div><div class="label">RY</div><div class="value" id="ry">0</div></div>
      </div>
    </div>
    <div class="card">
      <h2>Buttons</h2>
      <div class="value" id="buttons">0x0000</div>
    </div>
    <div class="card">
      <h2>ESP-NOW</h2>
      <div id="status" class="status">Waiting...</div>
      <p>Packets sent:<span id="packets">0</span>
      </p>
    </div>
  </div>
  <script>
    async function updateData() {
      try {
        const response = await fetch('/data');
        const data = await response.json();
        document.getElementById('lx').textContent = data.lx;
        document.getElementById('ly').textContent = data.ly;
        document.getElementById('rx').textContent = data.rx;
        document.getElementById('ry').textContent = data.ry;
        document.getElementById('buttons').textContent = '0x' + data.buttons.toString(16).padStart(4, '0').toUpperCase();
        document.getElementById('packets').textContent = data.packets;
        const status = document.getElementById('status');
        if (data.success) { status.textContent = 'Connected'; status.className = 'status success'; } 
        else { status.textContent = 'Send failed'; status.className = 'status failed'; }
      }
      catch (error) { console.log(error); }
    }
    setInterval(updateData, 100);
    updateData();
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleData() {
  char json[200];

  snprintf(
    json,
    sizeof(json),

    "{\"lx\":%d,\"ly\":%d,\"rx\":%d,\"ry\":%d,"
    "\"buttons\":%u,\"success\":%s,\"packets\":%lu}",

    latestFrame.lx,
    latestFrame.ly,
    latestFrame.rx,
    latestFrame.ry,

    latestFrame.buttons,

    lastSendSuccess ? "true" : "false",
    packetsSent);

  server.send(200, "application/json", json);
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

#if ENABLE_WEB_SERVER
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, WIFI_CHANNEL);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
#endif

  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

#if ENABLE_WEB_SERVER
  peerInfo.ifidx = WIFI_IF_AP;
#else
  peerInfo.ifidx = WIFI_IF_STA;
#endif

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer!");
    return;
  }

  Serial.println("Connecting to PS2 Controller...");

  int error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, true, false);

  if (error == 0) {
    Serial.println("Success! Controller configured.");
  } else {
    Serial.print("Controller connection failed: ");
    Serial.println(error);
  }

  Serial.println("Calibrating joystick centers...");

  lxCenter = getAverage(PSS_LX);
  lyCenter = getAverage(PSS_LY);
  rxCenter = getAverage(PSS_RX);
  ryCenter = getAverage(PSS_RY);

  Serial.println("Calibration complete!");
}

void loop() {
#if ENABLE_WEB_SERVER
  server.handleClient();
#endif
  latestFrame = readController();

  esp_err_t result = esp_now_send(receiverMac, (uint8_t*)&latestFrame, sizeof(latestFrame));

  if (result != ESP_OK) {
    Serial.print("Send error: ");
    Serial.println(result);
  }

  Serial.printf("WEB_SERVER(%d), SUCCESS(%d), COUNT(%lu), LX(%04d), LY(%04d), RX(%04d), RY(%04d), BUTTONS(0x%04X)\n", ENABLE_WEB_SERVER, lastSendSuccess, packetsSent, latestFrame.lx, latestFrame.ly, latestFrame.rx, latestFrame.ry, latestFrame.buttons);

  delay(50);
}