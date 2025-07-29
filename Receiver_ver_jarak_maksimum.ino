#include <SPI.h>                         // Library untuk komunikasi SPI, dibutuhkan LoRa RFM95W
#include <LoRa.h>                        // Library utama LoRa
#include <Wire.h>                        // Library untuk komunikasi I2C (digunakan oleh LCD)
#include <LiquidCrystal_I2C.h>          // Library LCD I2C

#define SS 5                             // CS/SS pin LoRa dihubungkan ke GPIO 5 ESP32
#define RST 14                           // RESET pin LoRa dihubungkan ke GPIO 14 ESP32
#define DIO0 2                           // DIO0 pin LoRa dihubungkan ke GPIO 2 ESP32

LiquidCrystal_I2C lcd(0x27, 20, 4);      // Inisialisasi LCD I2C dengan alamat 0x27 dan ukuran 20x4

unsigned long lastRepeatTime = 0;        // Waktu terakhir repeater aktif
const unsigned long REPEATER_TIMEOUT = 5000; // Batas waktu 5 detik menunggu repeater

void setup() {
  Serial.begin(9600);                    // Memulai komunikasi serial
  lcd.init();                            // Inisialisasi LCD
  lcd.backlight();                       // Menyalakan lampu latar LCD
  lcd.setCursor(0, 0);                   // Posisikan kursor di awal
  lcd.print("Receiver Siap...");         // Tampilkan pesan awal
  delay(2000);                           // Tunggu 2 detik
  lcd.clear();                           // Bersihkan LCD

  LoRa.setPins(SS, RST, DIO0);           // Set pin LoRa
  if (!LoRa.begin(915E6)) {              // Mulai LoRa di frekuensi 915 MHz
    Serial.println("❌ LoRa gagal mulai");
    lcd.setCursor(0, 0);
    lcd.print("LoRa Error!");
    while (true);                        // Berhenti di sini jika gagal
  }

  Serial.println("✅ LoRa Receiver aktif");
}

void loop() {
  int packetSize = LoRa.parsePacket();                     // Cek apakah ada paket masuk
  if (packetSize) {
    String received = LoRa.readStringUntil('\n');          // Baca isi paket sampai newline
    received.trim();                                       // Hilangkan spasi kosong

    Serial.print("📨 Data diterima mentah: [");
    Serial.print(received);
    Serial.println("]");

    if (received.length() < 10) {                          // Jika data terlalu pendek
      Serial.println("❌ Data terlalu pendek, abaikan");
      return;
    }

    unsigned long now = millis();                          // Ambil waktu sekarang

    if (received.startsWith("REPEAT|")) {                  // Jika dari repeater
      lastRepeatTime = now;                                // Catat waktu aktif repeater
      String rssi = getDataValue(getValue(received, '|', 1));
      String snr  = getDataValue(getValue(received, '|', 2));
      String loss = getDataValue(getValue(received, '|', 3));

      Serial.println("📥 Dari REPEATER:");
      Serial.println("RSSI : " + rssi + " dBm");
      Serial.println("SNR  : " + snr + " dB");
      Serial.println("LOSS : " + loss + " %");
      Serial.println("------------------------");

      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("DARI : REPEATER");           // Baris 1
      lcd.setCursor(0, 1); lcd.print("RSSI : " + rssi + " dBm");   // Baris 2
      lcd.setCursor(0, 2); lcd.print("SNR  : " + snr + " dB");     // Baris 3
      lcd.setCursor(0, 3); lcd.print("LOSS : " + loss + " %");     // Baris 4

      delay(2000);                                                 // Delay agar terbaca
    }

    else if (received.startsWith("TX|")) {                         // Jika dari transmitter langsung
      if (now - lastRepeatTime < REPEATER_TIMEOUT) {
        Serial.println("⏳ Abaikan TX (repeater masih aktif)");
        return;
      }

      int jumlahToken = countToken(received, '|');
      if (jumlahToken < 4) {
        Serial.println("❌ Jumlah bagian tidak cukup, abaikan");
        return;
      }

      String id   = getDataValue(getValue(received, '|', 1));
      String lat  = getDataValue(getValue(received, '|', 2));
      String lon  = getDataValue(getValue(received, '|', 3));
      String stat = getDataValue(getValue(received, '|', 4));

      if (id == "" || lat == "" || lon == "" || stat == "") {
        Serial.println("❌ Parsing gagal, data kosong");
        return;
      }

      Serial.println("📥 Dari TX (langsung):");
      Serial.println("ID     : " + id);
      Serial.println("LAT    : " + lat);
      Serial.println("LON    : " + lon);
      Serial.println("STATUS : " + stat);
      Serial.println("------------------------");

      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("DARI : TX LANGSUNG");         // Baris 1
      lcd.setCursor(0, 1); lcd.print("ID: " + id + " " + stat);     // Baris 2
      lcd.setCursor(0, 2); lcd.print("LAT: " + lat.substring(0, 8));// Baris 3
      lcd.setCursor(0, 3); lcd.print("LON: " + lon.substring(0, 8));// Baris 4

      delay(2000);
    }

    else {
      Serial.println("❌ Format tidak dikenali: " + received);
    }
  }
}

// Fungsi untuk mengambil nilai dari bagian tertentu string yang dipisahkan separator
String getValue(String data, char separator, int index) {
  int found = 0, start = 0, end = -1;
  for (int i = 0; i <= index; i++) {
    start = end + 1;
    end = data.indexOf(separator, start);
    if (end == -1) end = data.length();
  }
  return data.substring(start, end);
}

// Fungsi untuk mengambil data setelah titik dua, contoh: "RSSI:-80" → "-80"
String getDataValue(String input) {
  int colon = input.indexOf(':');
  if (colon != -1) {
    return input.substring(colon + 1);
  }
  return "";
}

// Fungsi untuk menghitung jumlah token/pemisah dalam string
int countToken(String str, char separator) {
  int count = 0;
  for (int i = 0; i < str.length(); i++) {
    if (str.charAt(i) == separator) count++;
  }
  return count;
}
