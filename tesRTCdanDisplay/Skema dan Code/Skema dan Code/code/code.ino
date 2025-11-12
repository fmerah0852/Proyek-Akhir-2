#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Pastikan alamat I2C-nya 0x27 atau 0x3F
RTC_DS3231 rtc;

void setup() {
  Wire.begin(21, 22);       // SDA = GPIO 21, SCL = GPIO 22
  Serial.begin(9600);

  lcd.begin(16, 2);         // <- ini perbaikannya!
  lcd.backlight();

  if (!rtc.begin()) {
    Serial.println("RTC tidak terdeteksi!");
    while (1);
  }

  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // optional, set waktu
}

void loop() {
  DateTime now = rtc.now();

  lcd.setCursor(0, 0);
  lcd.print("Tanggal: ");
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());

  lcd.setCursor(0, 1);
  lcd.print("Jam: ");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute());
  lcd.print(":");
  lcd.print(now.second());

  delay(1000);
}
