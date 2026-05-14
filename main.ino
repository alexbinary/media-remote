#include <BleKeyboard.h>

#define BAT_PIN A0
const float vbatmin = 2.75;
const float vbatmax = 4.20;
const int batteryUpdatePeriodMs = 1 * 60 * 1000;
unsigned long millis_lastBatteryUpdate = 0;

const int numButtons = 8;
const int buttons_pins[numButtons] = {D3,D4,D5,D6,D7,D8,D9,D10};
const int buttons_keys[numButtons] = {' ', KEY_RIGHT_ARROW, 'b', 'c', 'v', KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_UP_ARROW};
int button_previousState[numButtons] = {LOW};

BleKeyboard bleKeyboard("Media Remote");


void setup() {

  Serial.begin(9600);
  Serial.println("Hello, world!");

  pinMode(BAT_PIN, INPUT);
  for(int i=0; i<numButtons; i++) {
    pinMode(buttons_pins[i], INPUT_PULLDOWN);
  }

  bleKeyboard.begin();
  Serial.println("Bluetooth keyboard waiting for connections...");

  while(!bleKeyboard.isConnected());
  Serial.println("Connected");
  updateBatteryLevel();
}


void updateBatteryLevel() {

  Serial.println("Reading battery level...");

  const float mes = analogReadMilliVolts(BAT_PIN);
  const float vbat = mes * 2 / 1000;
  const int percent = (vbat - vbatmin) / (vbatmax - vbatmin) * 100;

  Serial.print(mes);
  Serial.print(" - ");
  Serial.print(vbat);
  Serial.print("V - ");
  Serial.print(percent);
  Serial.println("%");

  bleKeyboard.setBatteryLevel(percent);
}


void loop() {

  // poll buttons state
  for (int i=0; i<numButtons; i++) {
    int buttonState = digitalRead(buttons_pins[i]);
    if (buttonState != button_previousState[i]) {
      button_previousState[i] = buttonState;
      if(buttonState == HIGH) {
        bleKeyboard.write(buttons_keys[i]);
      }
    }
  }

  // update battery level every minute
  if (millis() - millis_lastBatteryUpdate > batteryUpdatePeriodMs) {
    updateBatteryLevel();
    millis_lastBatteryUpdate = millis();
  }

  // delay to save battery
  delay(100);
}