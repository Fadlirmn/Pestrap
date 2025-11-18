#include <WiFi.h>
#include <SPI.h>
#include <LoRa.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <RTClib.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= KONFIGURASI JARINGAN =================
#define WIFI_SSID "Pestrap"
#define WIFI_PASSWORD "12345678"
#define API_KEY "AIzaSyDbtL7jHyQYRD4NiWSJELp8doinnL5U1vM"
#define DATABASE_URL "https://pestrap-demo-default-rtdb.asia-southeast1.firebasedatabase.app/"

// ================= OBJEK FIREBASE =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= RTC =================
RTC_DS3231 rtc;  // Ganti RTC_DS1307 jika pakai DS1307

// ================= PIN LORA =================
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 26
#define LORA_FREQ 433E6

// ================= PIN I2C RTC =================
#define SDA_PIN 21
#define SCL_PIN 22

// ================= VARIABEL =================
String LoRaData = "";
unsigned long lastReceiveTime = 0;

// ================= FUNGSI SETUP =================
void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("🔌 Menghubungkan WiFi");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    Serial.print(".");
    delay(500);
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi tersambung");
    Serial.println("📶 IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi gagal tersambung");
  }
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Sign up anonymous
  Firebase.signUp(&config, &auth, "", "");
  
  // Assign callback untuk token generation
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("🔥 Firebase initialized");
}

void setupRTC() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!rtc.begin()) {
    Serial.println("❌ RTC tidak terdeteksi! Cek wiring.");
    while (1);
  }
  
  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC kehilangan daya, set waktu ke waktu kompilasi!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  DateTime now = rtc.now();
  Serial.printf("🕐 RTC OK. Waktu: %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
}

void setupLoRa() {
  Serial.println("📡 Inisialisasi LoRa...");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("❌ Gagal inisialisasi LoRa! Cek wiring.");
    while (1);
  }
  
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  
  Serial.println("✅ LoRa siap di 433 MHz (RECEIVER MODE)");
}

// ================= FUNGSI UTILITY =================
String getTimeStamp() {
  DateTime now = rtc.now();
  char buffer[25];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(buffer);
}

void sendToFirebase(String data, int rssi, float snr) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi tidak tersambung, skip Firebase");
    return;
  }
  
  if (!Firebase.ready()) {
    Serial.println("⚠️ Firebase belum siap");
    return;
  }
  
  String timestamp = getTimeStamp();
  String path = "/devices/perangkat2/lastData";
  
  FirebaseJson json;
  json.set("timestamp", timestamp);
  json.set("payload", data);
  json.set("rssi", rssi);
  json.set("snr", snr);
  
  if (Firebase.RTDB.setJSON(&fbdo, path, &json)) {
    Serial.println("✅ Firebase upload OK");
  } else {
    Serial.println("❌ Firebase gagal: " + fbdo.errorReason());
  }
}

// ================= SETUP DAN LOOP =================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║  ESP32 LoRa Gateway with RTC  ║");
  Serial.println("║      RECEIVER MODE            ║");
  Serial.println("╚════════════════════════════════╝\n");
  
  setupRTC();
  setupWiFi();
  setupFirebase();
  setupLoRa();
  
  Serial.println("\n🚀 Sistem siap menerima data LoRa...");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    LoRaData = "";
    
    // Baca data LoRa
    while (LoRa.available()) {
      LoRaData += (char)LoRa.read();
    }
    
    // Ambil info sinyal
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    String timestamp = getTimeStamp();
    
    // Tampilkan di Serial Monitor
    Serial.println("╔═══════════════ LoRa Message ═══════════════╗");
    Serial.println("│ Data     : " + LoRaData);
    Serial.println("│ Waktu    : " + timestamp);
    Serial.println("│ RSSI     : " + String(rssi) + " dBm");
    Serial.println("│ SNR      : " + String(snr) + " dB");
    Serial.println("╚═══════════════════════════════════════════╝");
    
    // Kirim ke Firebase
    sendToFirebase(LoRaData, rssi, snr);
    
    Serial.println();
    lastReceiveTime = millis();
  }
  
  delay(10);
}