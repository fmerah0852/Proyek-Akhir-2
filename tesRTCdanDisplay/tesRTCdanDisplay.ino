#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h> // Include the LiquidCrystal_I2C library

RTC_DS3231 rtc; // Create an instance of the RTC_DS3231 class
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the I2C LCD with its address (0x27 for most displays) and dimensions (16x2)

void setup() {
  lcd.init();         // Ganti dari lcd.begin() ke lcd.init()
  lcd.backlight();    // Nyalakan backlight
  lcd.clear();        // Bersihkan layar
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  Serial.begin(115200);
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  rtc.adjust(DateTime(__DATE__, __TIME__));
}


void loop() {
  // Read the current date and time from the DS3231
  DateTime now = rtc.now();
  
  // Clear the LCD
  lcd.clear();
  
  // Print the date and time on the LCD
  lcd.setCursor(0, 0); // Set the cursor to the top-left corner
  lcd.print("Date: ");
  lcd.print(now.year(), DEC);
  lcd.print("/");
  printTwoDigits(now.month());
  lcd.print("/");
  printTwoDigits(now.day());

  lcd.setCursor(0, 1); // Set the cursor to the second row
  lcd.print("Time: ");
  printTwoDigits(now.hour());
  lcd.print(":");
  printTwoDigits(now.minute());
  lcd.print(":");
  printTwoDigits(now.second());
  lcd.print("  ");

  // Print the same information to Serial
  Serial.print("Current Date: ");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  printTwoDigits(now.month());
  Serial.print("/");
  printTwoDigits(now.day());
  Serial.print("  Current Time: ");
  printTwoDigits(now.hour());
  Serial.print(":");
  printTwoDigits(now.minute());
  Serial.print(":");
  printTwoDigits(now.second());
  Serial.println();

  delay(1000); // Update every 1 second
}

void printTwoDigits(int number) {
  if (number < 10) {
    lcd.print("0"); // Print a leading zero for single-digit numbers
  }
  lcd.print(number);
}