#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin LoRa
#define SS 5
#define RST 14
#define DIO0 2

// LCD I2C 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Waktu prioritas repeater (5 detik)
unsigned long lastRepeatTime = 0;
const unsigned long REPEATER_TIMEOUT = 5000;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Receiver Ready");
  delay(2000);
  lcd.clear();

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("❌ LoRa gagal mulai");
    lcd.setCursor(0, 0);
    lcd.print("LoRa Error!");
    while (true);
  }

  Serial.println("✅ Receiver siap");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    received.trim();

    unsigned long now = millis();
    String source = "";

    // === Identifikasi asal data ===
  if (received.startsWith("REPEAT|")) {
    lastRepeatTime = now;
    Serial.println("✅ Deteksi data dari REPEATER");
    received = received.substring(7);
    source = "Repeater";
}

    }
    else if (received.startsWith("TX|")) {
      if (now - lastRepeatTime < REPEATER_TIMEOUT) {
        Serial.println("⏳ Abaikan TX (repeater aktif)");
        return; // Abaikan data dari TX karena repeater masih aktif
      }
      received = received.substring(3); // Hapus "TX|"
      source = "Transmitter";
    }
    else {
      Serial.println("❌ Format tidak dikenali");
      return;
    }

    Serial.println("📥 Diterima: " + received);
    Serial.println("SUMBER  : " + source);

    // === Ambil field GPS metadata ===
    String id    = getDataValue(getValue(received, '|', 0)); // ID
    String lat   = getDataValue(getValue(received, '|', 1)); // LAT
    String lon   = getDataValue(getValue(received, '|', 2)); // LON
    String stat  = getDataValue(getValue(received, '|', 3)); // STATUS

    // Cek apakah lengkap
    if (id == "" || lat == "" || lon == "" || stat == "") {
      Serial.println("❌ Data tidak lengkap!");
      return;
    }

    // Tampilkan ke Serial
    Serial.println("ID     : " + id);
    Serial.println("LAT    : " + lat);
    Serial.println("LON    : " + lon);
    Serial.println("STATUS : " + stat);
    Serial.println("------------------------------");

    // === Tampilkan ke LCD per bagian ===
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("ID:" + id);
    lcd.setCursor(0, 1); lcd.print("SRC:" + source);
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("LAT:" + lat.substring(0,6));
    lcd.setCursor(0, 1); lcd.print("LON:" + lon.substring(0,6));
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("STATUS:");
    lcd.setCursor(0, 1); lcd.print(stat);
    delay(2000);
  }
}

// Ambil bagian dari string dengan delimiter '|'
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

// Ambil VALUE dari format "KEY:VALUE"
String getDataValue(String input) {
  int colon = input.indexOf(':');
  if (colon != -1 && colon < input.length() - 1) {
    return input.substring(colon + 1);
  }
  return "";
}
