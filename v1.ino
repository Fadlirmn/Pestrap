#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "ir_counter.h"

// === WiFi & Firebase ===
#define WIFI_SSID "KEMENIOT"
#define WIFI_PASSWORD "zakiyaBusik2"
#define API_KEY "AIzaSyDbtL7jHyQYRD4NiWSJELp8doinnL5U1vM"
#define DATABASE_URL "https://pestrap-demo-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// === RTC DS3231 ===
RTC_DS3231 rtc;

// === Relay ===
#define RELAY_PIN 25   // aktif HIGH
bool relayState = false;

// === Timer ===
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // kirim setiap 10 detik

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  setupIR();

  // --- RTC ---
  if (!rtc.begin()) {
    Serial.println("❌ RTC tidak terdeteksi!");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC kehilangan daya, set waktu manual dulu!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // pakai waktu compile
  }

  // --- WiFi ---
  Serial.print("🔌 Menghubungkan ke WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n✅ WiFi tersambung");

  // --- Firebase ---
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("🔥 Sistem siap jalan pakai RTC lokal");
}

void loop() {
  updateIR();

  DateTime now = rtc.now();
  int hourNow = now.hour();
  bool shouldRelayOn = (hourNow >= 18 || hourNow < 6);
  digitalWrite(RELAY_PIN, shouldRelayOn ? LOW : HIGH);

  if (relayState != shouldRelayOn) {
    relayState = shouldRelayOn;
    Serial.printf("Relay: %s (pukul %02d:%02d)\n",
                  relayState ? "ON (malam)" : "OFF (siang)",
                  hourNow, now.minute());
  }

  if (Firebase.ready() && millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();
    int irVal = getIRCount();

    String path = "/device1/";
    String waktu = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) +
                   " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

    bool ok1 = Firebase.RTDB.setInt(&fbdo, path + "irCount", irVal);
    bool ok2 = Firebase.RTDB.setString(&fbdo, path + "time", waktu);
    bool ok3 = Firebase.RTDB.setInt(&fbdo, path + "relay", relayState);

    if (ok1 && ok2 && ok3) {
      Serial.printf("✅ Firebase update: IR=%d | Relay=%d | %s\n", irVal, relayState, waktu.c_str());
    } else {
      Serial.printf("❌ Firebase gagal: %s\n", fbdo.errorReason().c_str());
    }
  }

  delay(20);
}
