#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

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

void onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != sizeof(ControllerFrame)) {
    Serial.print("Invalid packet size: ");
    Serial.println(len);
    return;
  }

  ControllerFrame frame;

  memcpy(&frame, data, sizeof(frame));

  Serial.printf(
    "UP(%d), RIGHT(%d), DOWN(%d), LEFT(%d), TRIANGLE(%d), CIRCLE(%d), CROSS(%d), SQUARE(%d), L1(%d), L2(%d), L3(%d), R1(%d), R2(%d), R3(%d), SELECT(%d), START(%d), LX(%d), LY(%d), RX(%d), RY(%d)\n",

    (frame.buttons & BTN_UP) != 0,
    (frame.buttons & BTN_RIGHT) != 0,
    (frame.buttons & BTN_DOWN) != 0,
    (frame.buttons & BTN_LEFT) != 0,

    (frame.buttons & BTN_TRIANGLE) != 0,
    (frame.buttons & BTN_CIRCLE) != 0,
    (frame.buttons & BTN_CROSS) != 0,
    (frame.buttons & BTN_SQUARE) != 0,

    (frame.buttons & BTN_L1) != 0,
    (frame.buttons & BTN_L2) != 0,
    (frame.buttons & BTN_L3) != 0,

    (frame.buttons & BTN_R1) != 0,
    (frame.buttons & BTN_R2) != 0,
    (frame.buttons & BTN_R3) != 0,

    (frame.buttons & BTN_SELECT) != 0,
    (frame.buttons & BTN_START) != 0,

    frame.lx,
    frame.ly,
    frame.rx,
    frame.ry);

  Serial.println();
}

void setup() {
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("ESP-NOW Receiver");
  Serial.println("================");

  WiFi.mode(WIFI_STA);

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
  delay(10);
}