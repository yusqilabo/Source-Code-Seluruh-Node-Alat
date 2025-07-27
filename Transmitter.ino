#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Objek GPS dan konfigurasi port serial GPS
TinyGPSPlus gps;
HardwareSerial SerialGPS(1); // UART1: RX = GPIO16, TX = GPIO17

// Konfigurasi pin LoRa
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // Interval kirim 5 detik

int packetID = 0; // ID paket (digunakan untuk identifikasi & packet loss)

void setup() {
  Serial.begin(9600); // Serial monitor
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17); // GPS via UART1

  // Setup LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("❌ LoRa gagal mulai");
    while (1); // Stop di sini jika LoRa gagal
  }

  LoRa.setTxPower(20); // Daya maksimum
  Serial.println("✅ Transmitter siap");
}

void loop() {
  // Ambil data dari GPS jika tersedia
  while (SerialGPS.available()) {
    gps.encode(SerialGPS.read());
  }

  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;

    // Pastikan data GPS valid dan tidak terlalu lama (>5 detik)
    if (gps.location.isValid() && gps.location.age() < 5000) {
      float lat = gps.location.lat(); // Latitude real dari satelit
      float lon = gps.location.lng(); // Longitude real dari satelit

      // Format data yang akan dikirim
      String data = "TX|ID:" + String(packetID) +
                    "|LAT:" + String(lat, 6) +
                    "|LON:" + String(lon, 6) +
                    "|STATUS:VALID";

      // Kirim data via LoRa
      LoRa.beginPacket();
      LoRa.print(data);
      LoRa.endPacket();

      // Tampilkan ke Serial Monitor
      Serial.println("📤 Kirim: " + data);

      packetID++; // Increment ID untuk setiap paket
    } else {
      // Jika data GPS tidak valid → jangan kirim apa pun
      Serial.println("⚠️ GPS belum valid, tunggu sinyal...");
    }
  }
}
