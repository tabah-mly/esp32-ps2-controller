#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define LED_PIN 4

#define FRAME_DEBUG false

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

struct ControllerFrame {
  int8_t lx;
  int8_t ly;
  int8_t rx;
  int8_t ry;

  uint16_t buttons;
};

ControllerFrame receivedFrame = {};
volatile bool newData = false;

void onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != sizeof(ControllerFrame)) {
    return;
  }
  memcpy(&receivedFrame, data, sizeof(ControllerFrame));
  newData = true;
}

void actions() {
  bool upPressed = (receivedFrame.buttons & BTN_UP) != 0;
  bool rightPressed = (receivedFrame.buttons & BTN_RIGHT) != 0;
  bool downPressed = (receivedFrame.buttons & BTN_DOWN) != 0;
  bool leftPressed = (receivedFrame.buttons & BTN_LEFT) != 0;

  bool trianglePressed = (receivedFrame.buttons & BTN_TRIANGLE) != 0;
  bool circlePressed = (receivedFrame.buttons & BTN_CIRCLE) != 0;
  bool crossPressed = (receivedFrame.buttons & BTN_CROSS) != 0;
  bool squarePressed = (receivedFrame.buttons & BTN_SQUARE) != 0;

  bool L1Pressed = (receivedFrame.buttons & BTN_L1) != 0;
  bool L2Pressed = (receivedFrame.buttons & BTN_L2) != 0;
  bool L3Pressed = (receivedFrame.buttons & BTN_L3) != 0;

  bool R1Pressed = (receivedFrame.buttons & BTN_R1) != 0;
  bool R2Pressed = (receivedFrame.buttons & BTN_R2) != 0;
  bool R3Pressed = (receivedFrame.buttons & BTN_R3) != 0;

  bool selectPressed = (receivedFrame.buttons & BTN_SELECT) != 0;
  bool startPressed = (receivedFrame.buttons & BTN_START) != 0;

  int8_t lxDirection = receivedFrame.lx;
  int8_t lyDirection = receivedFrame.ly;
  int8_t rxDirection = receivedFrame.rx;
  int8_t ryDirection = receivedFrame.ry;

  if (FRAME_DEBUG) {
    Serial.printf(
      "UP(%d), RIGHT(%d), DOWN(%d), LEFT(%d), "
      "TRIANGLE(%d), CIRCLE(%d), CROSS(%d), SQUARE(%d), "
      "L1(%d), L2(%d), L3(%d), "
      "R1(%d), R2(%d), R3(%d), "
      "SELECT(%d), START(%d), "
      "LX(%d), LY(%d), RX(%d), RY(%d)\n",

      upPressed,
      rightPressed,
      downPressed,
      leftPressed,

      trianglePressed,
      circlePressed,
      crossPressed,
      squarePressed,

      L1Pressed,
      L2Pressed,
      L3Pressed,

      R1Pressed,
      R2Pressed,
      R3Pressed,

      selectPressed,
      startPressed,

      lxDirection,
      lyDirection,
      rxDirection,
      ryDirection);
  }

  Serial.println(L2Pressed);
  if (L2Pressed) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("Receiver");
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Waiting for controller...");
}

void loop() {
  if (newData) {
    newData = false;
    actions();
    delay(1);
  }
}