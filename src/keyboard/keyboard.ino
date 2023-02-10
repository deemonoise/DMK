#include <SoftwareSerial.h>
SoftwareSerial mySerial(11, 10); // RX, TX

#define BTN_AMOUNT 16

#include <EncButton2.h>
EncButton2<EB_BTN> btn[BTN_AMOUNT];

#include <Wire.h>

byte btnPins[BTN_AMOUNT] = {2, 3, 4, 5, 6, 7, 8, 9, 11, 12, A0, A1, A2, A3, A4, A5};

struct Str {
  byte action;
  byte button;
};

void setup() {
  mySerial.begin(4800);
  for (int i = 0; i < BTN_AMOUNT; i++) btn[i].setPins(INPUT_PULLUP, btnPins[i]);
}

void loop() {
  Str buf;
  for (int i = 0; i < BTN_AMOUNT; i++) btn[i].tick();
  for (int i = 0; i < BTN_AMOUNT; i++) {
    if (btn[i].press()) {
      buf.action = 1;
      buf.button = i;
      mySerial.write((byte*)&buf, sizeof(buf));
    }

    if (btn[i].release()) {
      digitalWrite(LED_BUILTIN, LOW);
      buf.action = 0;
      buf.button = i;
      mySerial.write((byte*)&buf, sizeof(buf));
    }
  }
}