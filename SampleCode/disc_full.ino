#include "FS.h"
#include "SD.h"
#include "SPI.h"

const int chipSelect = 10;
const int warningLedPin = 4; // Pin connected to your warning light

void setup() {
  Serial.begin(115200);
  pinMode(warningLedPin, OUTPUT);

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("Card Mount Failed");
    // Handle error: SD card missing or improperly connected
    return;
  }
  Serial.println("SD Card mounted successfully");
}

void loop() {
  // Calculate storage space
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();

  // Print to Serial Monitor
  Serial.printf("Used space: %llu MB\n", usedBytes / (1024 * 1024));
  Serial.printf("Total space: %llu MB\n", totalBytes / (1024 * 1024));

  // Calculate percentage (using float for accuracy)
  float percentUsed = ((float)usedBytes / (float)totalBytes) * 100.0;
  
  // Trigger warning light if disk is > 90% full
  if (percentUsed > 90.0) {
    digitalWrite(warningLedPin, HIGH); // Turn warning light on
    Serial.println("WARNING: Disk is over 90% full!");
  } else {
    digitalWrite(warningLedPin, HIGH); // Turn warning light off
    Serial.println("Disk space is safe.");
  }

  delay(10000); // Check space every 10 seconds
}