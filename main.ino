
#include <BleKeyboard.h>
#include <Preferences.h>
#include "esp_pm.h"


Preferences preferences;
BleKeyboard bleKeyboard("Media Remote");

bool previouslyConnected = false;

#define BAT_PIN A0

const float vbatmin = 2.75;
const float vbatmax = 4.20;

RTC_DATA_ATTR int bootCount = 0;

int incomingByte = 0;

const int numButtons = 8;
const int buttons_pins[numButtons] = {D1,D2,D3,D4,D5,D6,D7,D8};
const int buttons_keys[numButtons] = {KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW, ' ', 'c', 'v', 'b'};
typedef void (*InterruptHandler)();
void handleButtonD1();
void handleButtonD2();
const InterruptHandler buttons_isrs[numButtons] = {handleButtonD1, handleButtonD2};


int button_previousState[numButtons] = {LOW};

int buffer = -1;

unsigned long millis_lastBatteryUpdate = 0;


void setup() {

  Serial.begin(9600);
  Serial.println("Hello, world!");
  Serial.println("Boot number: " + String(++bootCount));
  print_wakeup_reason();

  esp_sleep_enable_ext1_wakeup((
    0
    // + (1ULL << GPIO_NUM_1)
    + (1ULL << GPIO_NUM_2)
  ), ESP_EXT1_WAKEUP_ANY_HIGH);

  esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 80,
    .min_freq_mhz = 40,
    .light_sleep_enable = true,
  };
  esp_pm_configure(&pm_config);

  pinMode(BAT_PIN, INPUT);
  for(int i=0; i<numButtons; i++) {
    pinMode(buttons_pins[i], INPUT_PULLDOWN);
  }

  // for(int i=0; i<2; i++) {
  //   attachInterrupt(digitalPinToInterrupt(buttons_pins[i]), buttons_isrs[i], RISING);
  // }

  bleKeyboard.begin();
  Serial.println("Bluetooth keyboard waiting for connections...");

  // waitConnected();
  // Serial.println("Connected");
  // updateBatteryLevel();

  // testPrefs();
}


void handleButtonD1() {

  buttonPress(0);
}


void handleButtonD2() {

  buttonPress(1);
}


void waitConnected() {

  while(!bleKeyboard.isConnected());
}


void testPrefs() {

  preferences.begin("my-app", false);
  unsigned int counter = preferences.getUInt("counter", 0);

  // Increase counter by 1
  counter++;

  // Print the counter to Serial Monitor
  Serial.printf("Current counter value: %u\n", counter);

  // Store the counter to the Preferences
  preferences.putUInt("counter", counter);

  // Close the Preferences
  preferences.end();
}


void print_wakeup_reason() {

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : {
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      int gpio_bitmask = esp_sleep_get_ext1_wakeup_status();
      Serial.print("GPIO: ");
      int gpio_num = log(gpio_bitmask)/log(2);
      Serial.println(gpio_num, 0);
      int btn_idx = gpio_num - 1;
      buttonPress(btn_idx);
      break;
    }
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
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


void sleep() {

  Serial.println("Going to sleep now");
  esp_deep_sleep_start();
  // esp_light_sleep_start();
}


void buttonPress(int i) {

  // if (i==0) { 
  //   sleep();
  // }

  int key = buttons_keys[i];
  Serial.print("Buffering key: ");
  Serial.println(key);

  buffer = key;

  sendBuffer();
}


void sendBuffer() {

  if (buffer != -1 && bleKeyboard.isConnected()) { 
    Serial.print("Sending buffer: ");
    Serial.println(buffer);

    bleKeyboard.write(buffer);
    buffer = -1;  
  }
}


void loop() {

  bool connected = bleKeyboard.isConnected();
  if(connected != previouslyConnected) {
    Serial.print("Status changed: ");

    if(connected) {
      Serial.println("connected");
      // delay(2000);
      // updateBatteryLevel();
    } else {
      Serial.println("disconnected");
    }

    previouslyConnected = connected;
  }


  for (int i=0; i<numButtons; i++) {

    int buttonState = digitalRead(buttons_pins[i]);
    if (buttonState != button_previousState[i]) {

      Serial.println(millis());

      if(buttonState == HIGH) {
        buttonPress(i);
      }

      button_previousState[i] = buttonState;
    }
  }


  sendBuffer();


  // update battery level every minute

  if (millis() - millis_lastBatteryUpdate > 1 * 60 * 1000) {

    updateBatteryLevel();
    millis_lastBatteryUpdate = millis();
  }


  // if (Serial.available() > 0) {

  //   incomingByte = Serial.read();

  //   Serial.print("I received: ");
  //   Serial.println(incomingByte, DEC);
  // }


  // button press never lasts more than 100ms

  delay(10);
}




