# Pestrap
Esp 32 Automation Pest detection
🌙 ESP32 Smart IR Counter & Relay Automation (Firebase + RTC + LoRa Ready)

Proyek ini adalah sistem otomatisasi berbasis ESP32 untuk menghitung deteksi objek menggunakan sensor infrared (IR), mengontrol relay berdasarkan waktu malam/pagi, dan mengirimkan data ke Firebase Realtime Database.
Sistem menggunakan RTC DS3231 sebagai sumber waktu lokal agar tetap bekerja walau tanpa koneksi internet.
Kode juga sudah disiapkan untuk integrasi LoRa RA-02 (410–525 MHz) antar node (master–slave).

⚙️ Fitur Utama

IR Counter
Mendeteksi dan menghitung jumlah objek yang lewat dengan debounce cerdas, hanya menghitung saat perubahan nyata pada sensor.
Relay Otomatis (Aktif LOW)
Hidup otomatis antara pukul 18:00 sampai 06:00.
Mati otomatis di luar rentang waktu malam.
Tidak tergantung deteksi IR.
RTC DS3231 (Tanpa NTP)
Menyimpan waktu secara mandiri, tetap akurat meskipun tanpa koneksi internet.
Firebase Realtime Database
Mengirim data setiap 10 detik:
Jumlah deteksi IR
Status relay
Timestamp dari RTC
Update realtime untuk monitoring jarak jauh.
LoRa RA-02 Ready
Struktur kode disiapkan agar mudah ditambah fungsi komunikasi LoRa antar node (belum diaktifkan di versi ini).

🔩 Konfigurasi Pin ESP32
Komponen	Pin ESP32	Keterangan
Relay IN	GPIO25	Aktif HIGH
IR Sensor OUT	GPIO32	Output digital IR
RTC SDA	GPIO21	I2C Data
RTC SCL	GPIO22	I2C Clock
LoRa NSS	GPIO5	Chip Select
LoRa MOSI	GPIO23	SPI Data Out
LoRa MISO	GPIO19	SPI Data In
LoRa SCK	GPIO18	SPI Clock
LoRa RST	GPIO14	Reset
LoRa DIO0	GPIO26	Interrupt pin
Power	3.3V	Semua kecuali relay
Ground	GND	Semua ground harus nyatu

esp 8266
D0 → LoRa DIO0
D1 → I2C SCL
D2 → I2C SDA
D3 → Relay
D4 → LoRa RST
D5 → LoRa SCK
D6 → LoRa MISO
D7 → LoRa MOSI
D8 → LoRa NSS
v5 -> Vin
v3 -> v3
gnd-> gnd

🧰 Library yang Digunakan
WiFi.h
Wire.h
RTClib.h
Firebase_ESP_Client.h
addons/TokenHelper.h
addons/RTDBHelper.h
Pastikan library Firebase Arduino Client by Mobizt dan RTClib sudah terinstal di Arduino IDE.

🚀 Cara Kerja
ESP32 membaca waktu dari RTC DS3231.
IR sensor menghitung jumlah deteksi lewat (aktif LOW).
Relay otomatis aktif saat malam (18:00–06:00).
Data dikirim ke Firebase setiap 10 detik.
Semua log ditampilkan di Serial Monitor untuk debugging.

📡 Rencana Pengembangan
Integrasi LoRa antar node (master–slave) untuk sistem multi-sensor.
Logging data harian otomatis di Firebase.
Dashboard monitoring berbasis MIT App Inventor / Kodular.

👤 Pembuat

Dikembangkan oleh Fadlirmn Lab Project — dengan kombinasi logika, kesabaran, dan sedikit frustrasi terhadap sensor yang terlalu sensitif.
