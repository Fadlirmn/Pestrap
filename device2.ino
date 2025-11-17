#include <Wire.h>
#include <RTClib.h>
#include <VL53L1X.h>
#include <SPI.h>
#include <LoRa.h>

#define DEVICE_NAME "Device-2"

// Pin LoRa
#define LORA_SS   D8
#define LORA_RST  D4
#define LORA_DIO0 D0

// Relay
#define RELAY_PIN D3    // Active LOW

// Distance threshold for detection
int thresholdDist = 300;

// Counters & flags
unsigned long lastSendTime = 0;
unsigned long sendInterval = 60000; // 1 menit
bool detectLock = false;
int beetleCount = 0;

// Objects
RTC_DS3231 rtc;
VL53L1X sensor;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n--- DEVICE 2 System Init ---");

  // Relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Active LOW, default off

  // I2C
  Wire.begin(D2, D1);

  // RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
  }

  // VL53L1X
  sensor.setTimeout(500);
  if (!sensor.init()) {
    Serial.println("Failed to init VL53L1X");
    while (1) { delay(10); }
  }

  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(100000);
  sensor.startContinuous(100);

  // LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed!");
    while (1) { delay(100); }
  }

  Serial.println("System ready.\n");
}

void handleRelay() {
  DateTime now = rtc.now();
  int hourNow = now.hour();

  if (hourNow >= 18 || hourNow < 6) {
    digitalWrite(RELAY_PIN, LOW);  // Lamp ON
  } else {
    digitalWrite(RELAY_PIN, HIGH); // Lamp OFF
  }
}

void sendLoRa() {
  DateTime now = rtc.now();
  char timestring[25];
  sprintf(timestring, "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());

  String payload = "{";
  payload += "\"device\":\"" + String(DEVICE_NAME) + "\",";
  payload += "\"count\":" + String(beetleCount) + ",";
  payload += "\"time\":\"" + String(timestring) + "\"";
  payload += "}";

  Serial.println("Sending LoRa:");
  Serial.println(payload);

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();  // async
}

void loop() {
  handleRelay();

  // read distance
  int distance = sensor.read();
  if (distance > 0 && distance < thresholdDist && !detectLock) {
    beetleCount++;
    detectLock = true;
    Serial.printf("Kumbang terdeteksi. Total: %d\n", beetleCount);
  }

  if (distance >= thresholdDist && detectLock) {
    detectLock = false;
  }

  // Send data periodically
  if (millis() - lastSendTime > sendInterval) {
    sendLoRa();
    lastSendTime = millis();
  }

  yield();
  delay(5);
}
