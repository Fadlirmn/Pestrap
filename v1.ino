#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <LoRa.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include "ir_sensor.h"  // file terpisah untuk logika IR

// ================= KONFIGURASI JARINGAN =================
#define WIFI_SSID "KEMENIOT"
#define WIFI_PASSWORD "zakiyaBusik2"
#define API_KEY "AIzaSyDbtL7jHyQYRD4NiWSJELp8doinnL5U1vM"
#define DATABASE_URL "https://pestrap-demo-default-rtdb.asia-southeast1.firebasedatabase.app/"

// ================= OBJEK FIREBASE =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= RTC =================
RTC_DS3231 rtc;

// ================= PIN =================
#define RELAY_PIN 25
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_FREQ 433E6

// ================= VARIABEL =================
bool relayState = false;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 detik

// ================= FUNGSI SETUP =================
void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("🔌 Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n✅ WiFi tersambung");
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void setupRTC() {
  if (!rtc.begin()) {
    Serial.println("❌ RTC tidak terdeteksi!");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC kehilangan daya, set waktu ke waktu kompilasi!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void setupLoRa() {
  Serial.println("🔄 Inisialisasi LoRa...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("❌ Gagal inisialisasi LoRa!");
    while (1);
  }
  Serial.println("✅ LoRa siap di 433 MHz");
}

void setupRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

// ================= FUNGSI LOGIKA =================
void updateRelay(DateTime now) {
  int hourNow = now.hour();
  bool shouldRelayOn = (hourNow >= 18 || hourNow < 6);
  digitalWrite(RELAY_PIN, shouldRelayOn ? LOW : HIGH);

  if (relayState != shouldRelayOn) {
    relayState = shouldRelayOn;
    Serial.printf("Relay: %s (pukul %02d:%02d)\n",
                  relayState ? "ON (malam)" : "OFF (siang)",
                  hourNow, now.minute());
  }
}

void sendFirebaseAndLoRa(DateTime now) {
  if (Firebase.ready() && millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();

    String waktu = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) +
                   " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

    bool ok1 = Firebase.RTDB.setInt(&fbdo, "/device1/irCount", irCount);
    bool ok2 = Firebase.RTDB.setString(&fbdo, "/device1/time", waktu);
    bool ok3 = Firebase.RTDB.setInt(&fbdo, "/device1/relay", relayState);

    if (ok1 && ok2 && ok3) {
      Serial.printf("🔥 Firebase update: IR=%d | Relay=%d | %s\n", irCount, relayState, waktu.c_str());
    } else {
      Serial.printf("❌ Firebase gagal: %s\n", fbdo.errorReason().c_str());
    }

    // Kirim ke LoRa
    String packet = "IR=" + String(irCount) + ",Relay=" + String(relayState) + ",Time=" + waktu;
    LoRa.beginPacket();
    LoRa.print(packet);
    LoRa.endPacket();
    Serial.println("📡 Data dikirim via LoRa: " + packet);
  }
}

// ================= SETUP DAN LOOP =================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=21, SCL=22

  setupRelay();
  setupIR();         // dari file ir_sensor.h
  setupRTC();
  setupWiFi();
  setupFirebase();
  setupLoRa();

  Serial.println("🚀 Sistem siap berjalan (MASTER MODE)");
}

void loop() {
  updateIR();        // dari file ir_sensor.h
  DateTime now = rtc.now();
  updateRelay(now);
  sendFirebaseAndLoRa(now);
  delay(20);
}
