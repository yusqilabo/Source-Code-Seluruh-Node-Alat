#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Konfigurasi pin LoRa
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 2

// LCD I2C 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2); // Gunakan 20, 4 jika pakai LCD 20x4

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Repeater Ready");

  // Inisialisasi LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("❌ Gagal inisialisasi LoRa!");
    lcd.setCursor(0, 1);
    lcd.print("LoRa Error!");
    while (true); // Berhenti jika gagal
  }

  LoRa.setTxPower(20); // Maksimal daya pancar
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tunggu paket...");
}

void loop() {
  int packetSize = LoRa.parsePacket(); // Cek apakah ada paket masuk
  if (packetSize) {
    String received = "";

    // Baca data masuk dari LoRa byte per byte
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }
    received.trim(); // Hapus spasi/kosong di depan & belakang

    Serial.println("📥 Diterima: " + received);

    // Cegah loop: Jangan teruskan data yang sudah mengandung "REPEAT|"
    if (received.startsWith("REPEAT|")) {
      Serial.println("⏭️ Sudah REPEAT → abaikan");
      return;
    }

    // Validasi: hanya teruskan data dari transmitter (format "TX|...")
    if (!received.startsWith("TX|")) {
      Serial.println("⚠️ Bukan dari Transmitter → abaikan");
      return;
    }

    // Tambahkan prefix "REPEAT|" sebelum meneruskan ke Receiver
    String forwarded = "REPEAT|" + received;

    delay(150); // Penundaan untuk hindari konflik di udara

    // Kirim ulang data ke receiver
    LoRa.beginPacket();
    LoRa.print(forwarded);
    LoRa.endPacket();

    Serial.println("📤 Diteruskan ke Receiver: " + forwarded);

    // Tampilkan ID di LCD untuk memantau data yang diteruskan
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RELAY > RX");

    // Tampilkan ID paket di baris ke-2
    String id = getDataValue(getValue(received, '|', 1)); // Posisi ID adalah elemen ke-1
    lcd.setCursor(0, 1);
    lcd.print("ID:");
    lcd.print(id);

    delay(1500); // Tampilan jeda
  }
}

// Ambil elemen string berdasarkan separator '|'
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

// Ambil nilai setelah tanda ':' dari format "KEY:VALUE"
String getDataValue(String input) {
  int colon = input.indexOf(':');
  if (colon != -1 && colon < input.length() - 1) {
    return input.substring(colon + 1);
  }
  return "";
}
