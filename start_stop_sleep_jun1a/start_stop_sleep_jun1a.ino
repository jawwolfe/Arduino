#include <Wire.h>
#include "RTClib.h"
#include "time.h"

RTC_DS3231 rtc;

// ================= CONFIGURATION =================
// Define your two daily wake-up windows (in 24-hour format)
const int START_1_HR = 8;   // Window 1 Start: 08:30
const int START_1_MIN = 30;
const int STOP_1_HR = 9;    // Window 1 End: 09:30
const int STOP_1_MIN = 30;

const int START_2_HR = 17;  // Window 2 Start: 17:00
const int START_2_MIN = 0;
const int STOP_2_HR = 18;   // Window 2 End: 18:00
const int STOP_2_MIN = 0;
// =================================================

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize

  // Initialize I2C with ESP32-S3 default pins (SDA=8, SCL=9)
  // Adjust these if your board uses different pins
  Wire.begin(8, 9); 

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC module! Check wiring.");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, letting's set the time!");
    // This sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Get current time from the DS3231 module
  DateTime now = rtc.now();
  
  // Synchronize ESP32's internal RTC with the hardware RTC
  struct tm tm_info;
  tm_info.tm_year = now.year() - 1900;
  tm_info.tm_mon = now.month() - 1;
  tm_info.tm_mday = now.day();
  tm_info.tm_hour = now.hour();
  tm_info.tm_min = now.minute();
  tm_info.tm_sec = now.second();
  
  timeval tv = { .tv_sec = mktime(&tm_info), .tv_usec = 0 };
  settimeofday(&tv, NULL);

  Serial.printf("Current RTC Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());

  // Convert everything to minutes since midnight for easy comparison
  int currentMinutes = (now.hour() * 60) + now.minute();
  int start1 = (START_1_HR * 60) + START_1_MIN;
  int stop1  = (STOP_1_HR * 60) + STOP_1_MIN;
  int start2 = (START_2_HR * 60) + START_2_MIN;
  int stop2  = (STOP_2_HR * 60) + STOP_2_MIN;

  // Check if we are currently INSIDE either of the two active windows
  bool inWindow1 = (currentMinutes >= start1 && currentMinutes < stop1);
  bool inWindow2 = (currentMinutes >= start2 && currentMinutes < stop2);

  if (inWindow1 || inWindow2) {
    // We are supposed to be awake! Proceed to void loop()
    Serial.println("Inside an active window. Running loop()...");
  } else {
    // We are outside the windows. Calculate sleep duration and go to sleep immediately.
    int minutesToSleep = 0;

    if (currentMinutes < start1) {
      // It's early morning, sleep until Window 1
      minutesToSleep = start1 - currentMinutes;
    } else if (currentMinutes < start2) {
      // We are between Window 1 and Window 2, sleep until Window 2
      minutesToSleep = start2 - currentMinutes;
    } else {
      // It's late night, sleep until Window 1 of the NEXT day
      minutesToSleep = (1440 - currentMinutes) + start1;
    }

    // Safeguard seconds adjustment: subtract current seconds so we wake up exactly on the minute mark
    long secondsToSleep = (minutesToSleep * 60) - now.second();
    if (secondsToSleep <= 0) secondsToSleep = 1; // Prevent negative/zero sleep

    Serial.printf("Outside active windows. Sleeping for %ld seconds...\n", secondsToSleep);
    Serial.flush();

    // Configure deep sleep timer (expects microseconds)
    esp_sleep_enable_timer_wakeup((uint64_t)secondsToSleep * 1000000ULL);
    esp_deep_sleep_start();
  }
}

void loop() {
  // Your main task goes here. It runs ONLY during the active windows.
  Serial.println("ESP32-S3 is awake and performing tasks...");
  delay(5000); 

  // Dynamically check if our active window has just expired
  DateTime now = rtc.now();
  int currentMinutes = (now.hour() * 60) + now.minute();
  int start1 = (START_1_HR * 60) + START_1_MIN;
  int stop1  = (STOP_1_HR * 60) + STOP_1_MIN;
  int start2 = (START_2_HR * 60) + START_2_MIN;
  int stop2  = (STOP_2_HR * 60) + STOP_2_MIN;

  bool inWindow1 = (currentMinutes >= start1 && currentMinutes < stop1);
  bool inWindow2 = (currentMinutes >= start2 && currentMinutes < stop2);

  // If we drop out of BOTH windows, force a restart so setup() can handle the deep sleep math
  if (!inWindow1 && !inWindow2) {
    Serial.println("Active window expired! Going to sleep.");
    Serial.flush();
    esp_restart(); 
  }
}
\\How It Works Behind the Scenes
\\The Time Sync: When the ESP32-S3 boots, it talks to the DS3231 via I2C to get the exact time and maps it to its internal chip clock.
\\The "Time Check" Guard: Before letting the code reach loop(), setup() does math using minutes since midnight.
\\Smart Sleep: If it's 10:00 AM (and the windows are 8:30-9:30 and 17:00-18:00), the code calculates that it needs to wait 7 hours (420 minutes) until 17:00. It sets a timer for exactly that long and plunges into deep sleep, turning off the CPU and Wi-Fi to save battery.
\\Active State: If it is inside a window, setup() completes, and loop() handles your code. At the end of every loop() cycle, it checks the time again. Once the window expires, it triggers esp_restart(), pushing it back to setup() where the sleep math calculation takes over seamlessly.
