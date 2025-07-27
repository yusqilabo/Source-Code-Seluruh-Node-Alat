#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi pin LoRa
#define SS 5
#define RST 14
#define DIO0 2

// LCD I2C 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Untuk menghitung Packet Loss
int lastID = -1;
int totalPacket = 0;
int lostPacket = 0;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Receiver Siap...");

  // Inisialisasi LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("❌ LoRa Gagal Mulai");
    lcd.setCursor(0, 1);
    lcd.print("LoRa Gagal Start");
    while (1);
  }

  delay(2000);
  lcd.clear();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";

    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    received.trim();

    Serial.println("📥 Diterima: " + received);

    // ❌ Abaikan jika bukan dari repeater
    if (!received.startsWith("REPEAT|")) {
      Serial.println("⛔ Abaikan: Bukan dari repeater");
      return;
    }

    // Hapus prefix "REPEAT|" sebelum parsing
    received = received.substring(7);

    // Ambil field ID dari format: TX|ID:123|LAT:...|LON:...|STATUS:...
    String idStr = getDataValue(getValue(received, '|', 1));  // TX|[ID:123]
    if (idStr == "") {
      Serial.println("❌ ID tidak ditemukan!");
      return;
    }

    int currentID = idStr.toInt();

    // Hitung packet loss
    if (lastID != -1 && currentID > lastID + 1) {
      lostPacket += (currentID - lastID - 1);
    }

    lastID = currentID;
    totalPacket++;

    // Dapatkan nilai RSSI dan SNR asli dari modul LoRa
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    float lossPercent = (totalPacket + lostPacket == 0) ? 0.0 : ((float)lostPacket / (totalPacket + lostPacket)) * 100.0;

    // Tampilkan ke Serial Monitor
    Serial.println("SUMBER : Repeater");
    Serial.print("ID     : "); Serial.println(currentID);
    Serial.print("RSSI   : "); Serial.print(rssi); Serial.println(" dBm");
    Serial.print("SNR    : "); Serial.print(snr, 2); Serial.println(" dB");
    Serial.print("LOSS   : "); Serial.print(lossPercent, 1); Serial.println(" %");
    Serial.println("------------------------");

    // Tampilkan ke LCD 20x4
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("ID   : "); lcd.print(currentID);
    lcd.setCursor(0, 1); lcd.print("RSSI : "); lcd.print(rssi); lcd.print(" dBm");
    lcd.setCursor(0, 2); lcd.print("SNR  : "); lcd.print(snr, 1); lcd.print(" dB");
    lcd.setCursor(0, 3); lcd.print("LOSS : "); lcd.print(lossPercent, 1); lcd.print(" %");

    delay(2000);
  }
}

// Ambil bagian string berdasarkan pemisah '|'
String getValue(String data, char separator, int index) {
  int found = 0;
  int start = 0;
  int end = -1;

  for (int i = 0; i <= index; i++) {
    start = end + 1;
    end = data.indexOf(separator, start);
    if (end == -1) end = data.length();
  }

  return data.substring(start, end);
}

// Ambil nilai setelah ':'
String getDataValue(String input) {
  int colonIndex = input.indexOf(':');
  if (colonIndex != -1) {
    return input.substring(colonIndex + 1);
  }
  return "";
}
