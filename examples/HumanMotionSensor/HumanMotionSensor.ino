/**
 * Piroelectric sensor
 *
 *  Created on: Feb. 8, 2022
 *
 */

#include <Arduino.h>
#include <time.h>
#include <ESP32Time.h>
#include <sys/time.h>
#include <esp_system.h>

#define MICROSEC 1000000

// Non-volatile valiables for main loop
RTC_DATA_ATTR struct st_main {
  uint32_t bootCount;     // boot count
  uint32_t now;           // now time
} main0;

//////////////////
// Piroelectric //
//////////////////

#define PIRS_IO GPIO_NUM_35  // piroelectric sensor port number
#define PIRS_OFF_SPAN 60     // delay time for Undetect: 60 [sec]
#define PIRS_DELAY 2         // delay time for next event: 2[sec]

// Non volatile valiables for piroelectric sensor
RTC_DATA_ATTR struct st_pirs {
  uint8_t st;             // state
  bool pirs;              // state of piroelectric sensor
  bool detect;            // detect
  uint32_t now;           // time
  uint32_t undetect_tmp;  // undetect time
} piroelectric;

void pirs_init() {
  pinMode(PIRS_IO, INPUT);
  piroelectric.pirs = LOW;
  piroelectric.detect = LOW;
  piroelectric.st = 0;
}

// return a current time
uint32_t now() {
  return (uint32_t)time(NULL);
}


//////////////////
////// main //////
//////////////////

//
// code for detected
//
void detected(void) {
  
  // put your code here



}

//
// code for undetected
//
void undetected(void) {

  // put your code here



}


//
// Boot by timer event
//
void timer_event() {
  main0.now = now();

  switch (piroelectric.st) {
    uint32_t delaytime;

    // Delay PIRS_OFF_SPAN from Undetected
    case 1:
      Serial.println("PIRS LOW DELAY");
      delaytime = (piroelectric.undetect_tmp + PIRS_OFF_SPAN - now()) * MICROSEC;
      Serial.print("Delaytime: ");
      Serial.println(delaytime);
      esp_sleep_enable_timer_wakeup(delaytime);
      esp_sleep_enable_ext0_wakeup(PIRS_IO, LOW);
      piroelectric.st = 2;
      break;

    // Fix Undetect After PIRS_OFF_SPAN
    case 2:
      Serial.println("PIRS LOW");
      esp_sleep_enable_ext0_wakeup(PIRS_IO, LOW);
      piroelectric.now = main0.now - PIRS_OFF_SPAN;
      piroelectric.pirs = LOW;
      piroelectric.st = 3;

      // Codes for Undetect
      Serial.println("***** UNDETECTED *****");
      undetected();

      break;

    // Continue undetect state
    case 3:
      Serial.println("PIRS STILL LOW");
      esp_sleep_enable_ext0_wakeup(PIRS_IO, LOW);
      break;

    // Detect
    case 4:
      Serial.println("PIRS STILL HIGH");
      esp_sleep_enable_ext0_wakeup(PIRS_IO, HIGH);
      break;

    // First boot
    default:
      esp_sleep_enable_ext0_wakeup(PIRS_IO, LOW);
      piroelectric.st = 3;
      break;
  }
}

// for first boot
//
void first_event() {

  // Initialize boot counter
  main0.bootCount = 0;

  Serial.println("-------- First boot --------");

  // Initialize variables for piroelectric sensor
  pirs_init();

  esp_sleep_enable_timer_wakeup(PIRS_DELAY * MICROSEC);
  main0.now = now();
}

// Event for piroelectric sensor
//
void pirs_event() {
  main0.now = now();

  if (piroelectric.detect == HIGH) {

    // Undetect piroelectric sensor
    piroelectric.undetect_tmp = main0.now;
    Serial.println("PIRS LOW DETECT");
    piroelectric.detect = LOW;
    piroelectric.st = 1;
    esp_sleep_enable_timer_wakeup(PIRS_DELAY * MICROSEC);

  } else {

    // Detect piroelectric sensor
    switch (piroelectric.st) {

      // Continue detected state
      case 2:
        Serial.println("PIRS LOW BREAKED");
        piroelectric.detect = HIGH;
        piroelectric.st = 4;
        esp_sleep_enable_timer_wakeup(PIRS_DELAY * MICROSEC);
        break;

      // Undetected to Detected
      case 3:
        Serial.println("PIRS HIGH");
        piroelectric.now = main0.now;
        piroelectric.detect = HIGH;
        piroelectric.pirs = HIGH;
        piroelectric.st = 4;
        esp_sleep_enable_timer_wakeup(PIRS_DELAY * MICROSEC);

        // Codes for detected
        Serial.println("***** DETECTED *****");
        detected();

        break;
    }
  }
}

///////////////////////////////////
/////////////// setup /////////////
///////////////////////////////////

void setup() {

  // Initialize serial port
  Serial.begin(115200);
  while (!Serial) delay(100);

  // boot count up
  main0.bootCount++;
  Serial.println("Boot count: " + String(main0.bootCount));

  // get boot reason cause
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.printf("Boot by External IO (RTC_IO)\n");
      pirs_event();
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.printf("Boot by External interrupt (RTC_CNTL) IO=%llX\n", esp_sleep_get_ext1_wakeup_status());
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.printf("Boot by timer interrupt.\n");
      timer_event();
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.printf("Boot by touch interrupt PAD=%d\n", esp_sleep_get_touchpad_wakeup_status());
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.printf("Boot by ULP program.\n");
      break;
    case ESP_SLEEP_WAKEUP_GPIO:
      Serial.printf("Boot by GPIO interrupt from light sleep state.\n");
      break;
    case ESP_SLEEP_WAKEUP_UART:
      Serial.printf("Boot by UART interrupt from light sleep state.\n");
      break;
    default:
      Serial.printf("Boot from other reason.\n");
      first_event();
      break;
  }

  // Flush serial port
  Serial.print("Bye!\n\n");
  Serial.flush();

  // into the deep sleep
  esp_deep_sleep_start();
}

void loop() {}
