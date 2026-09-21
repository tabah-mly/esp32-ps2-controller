#include <Arduino.h>
#include <PS2X_lib.h>
#include <WiFi.h>
#include <esp_now.h>

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

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, receiverMac, 6);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

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
  latestFrame = readController();

  esp_err_t result = esp_now_send(receiverMac, (uint8_t*)&latestFrame, sizeof(latestFrame));

  if (result != ESP_OK) {
    Serial.print("Send error: ");
    Serial.println(result);
  }

  Serial.printf("SUCCESS(%d), COUNT(%lu), LX(%04d), LY(%04d), RX(%04d), RY(%04d), BUTTONS(0x%04X)\n", lastSendSuccess, packetsSent, latestFrame.lx, latestFrame.ly, latestFrame.rx, latestFrame.ry, latestFrame.buttons);

  delay(50);
}