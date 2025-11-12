  #include <Wire.h>
  #include <RTClib.h>
  #include <LiquidCrystal_I2C.h>

  RTC_DS3231 rtc;
  LiquidCrystal_I2C lcd(0x27, 16, 2);

  const int relayPin = 23;
  const unsigned long interval = 60000; // 1 menit dalam milidetik
  unsigned long previousMillis = 0;
  unsigned long relayStartMillis = 0;
  bool relayState = false;

  void printTwoDigits(int number) {
    if (number < 10) {
      lcd.print("0");
    }
    lcd.print(number);
  }

  void setup() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");

    Serial.begin(115200);
    Wire.begin();

    if (!rtc.begin()) {
      Serial.println("Couldn't find RTC");
      while (1);
    }

    rtc.adjust(DateTime(__DATE__, __TIME__));

    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
  }

  void loop() {
    unsigned long currentMillis = millis();

    // Cek apakah waktunya toggle relay
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      relayState = !relayState;
      digitalWrite(relayPin, relayState ? HIGH : LOW);
      if (relayState) {
        relayStartMillis = currentMillis; // Catat waktu mulai relay ON
      }
    }

    lcd.clear();
    if (relayState) {                                                                                                                         
      // Hitung waktu tersisa
      

      DateTime now = rtc.now();

      lcd.setCursor(0, 0);
      lcd.print("Time: ");
      printTwoDigits(now.hour());
      lcd.print(":");
      printTwoDigits(now.minute());
      lcd.print(":");
      printTwoDigits(now.second());

      lcd.setCursor(0, 1);
      lcd.print("Alat tidak aktif");

      
    } else {
      unsigned long remaining = interval - (currentMillis - relayStartMillis);
      int secondsLeft = remaining / 1000;
      
      lcd.print("Alat Aktif : ");

      lcd.setCursor(0, 1);
      lcd.print("Waktu: ");
      printTwoDigits(secondsLeft / 60);
      lcd.print(":");
      printTwoDigits(secondsLeft % 60);
    }

    delay(1000);
  }
