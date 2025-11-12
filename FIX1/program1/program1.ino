#define BLYNK_TEMPLATE_ID "TMPL6tlPOaJPd"
#define BLYNK_TEMPLATE_NAME "Pakan Lele Otomatis dan Monitoring Kualitas Air"
#define BLYNK_AUTH_TOKEN "Ic2Jjz9IxTzDTONIIxtLOg0FXn5cPN8X"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <RTClib.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "TestingPA";
char pass[] = "inikelompok5";

#define PH_PIN 34
#define RELAY_PIN 19
#define SERVO_PIN 18
#define TRIG_PIN 26
#define ECHO_PIN 25

RTC_DS3231 rtc;
Servo myServo;
BlynkTimer timer;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Ganti alamat jika berbeda

// Blower state
bool blowerActive = false;
bool blowerAutoTriggered = false;
bool blowerAutoEnable = false;
unsigned long blowerManualStart = 0;
unsigned long blowerAutoStart = 0;
int blowerAutoDuration = 0;

// Jadwal blower otomatis
int autoBlowerHour = 0;
int autoBlowerMinute = 0;

// Nilai sensor terakhir
float lastPH = 0.0;
int lastKapasitas = 0;

// ==== BLYNK HANDLERS ====
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1 && !blowerActive) {
  digitalWrite(RELAY_PIN, LOW);  // ubah dari HIGH ke LOW
  myServo.write(90);
  blowerActive = true;
  blowerManualStart = millis();
} else if (value == 0 && blowerActive) {
  digitalWrite(RELAY_PIN, HIGH); // ubah dari LOW ke HIGH
  myServo.write(0);
  blowerActive = false;
}
}

BLYNK_WRITE(V4) { autoBlowerHour = param.asInt(); }
BLYNK_WRITE(V5) { autoBlowerMinute = param.asInt(); }
BLYNK_WRITE(V6) { blowerAutoDuration = param.asInt() * 1000; }
BLYNK_WRITE(V8) { blowerAutoEnable = param.asInt(); }

// === TEST NOTIFIKASI ===
BLYNK_WRITE(V9) {
  int testCode = param.asInt();
  switch (testCode) {
    case 1:
      Blynk.logEvent("pakan_habis", "Pakan hampir habis: 15%");
      break;
    case 2:
      Blynk.logEvent("blower_otomatis_on", "Blower otomatis dimulai");
      break;
    case 3:
      Blynk.logEvent("blower_otomatis_off", "Blower otomatis selesai");
      break;
    case 4:
      Blynk.logEvent("pakan_5menit", "Pemberian pakan sudah berlangsung 5 menit");
      break;
    default:
      break;
  }
}

// ==== SENSOR PH ====
void sendPH() {
  int sensorValue = analogRead(PH_PIN);
  float voltage = sensorValue * (3.3 / 4095.0);
  float ph = (voltage * 3.5); // Kalibrasi sederhana
  lastPH = ph;
  Blynk.virtualWrite(V3, ph);
}

// ==== SENSOR ULTRASONIK ====
long readPakanLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

void sendPakanLevel() {
  long distance = readPakanLevel();
  int kapasitas = map(distance, 5, 30, 100, 0); // sesuaikan nilai
  kapasitas = constrain(kapasitas, 0, 100);
  lastKapasitas = kapasitas;
  Blynk.virtualWrite(V7, kapasitas);
  if (kapasitas < 20) {
    Blynk.logEvent("pakan_habis", "Pakan hampir habis: " + String(kapasitas) + "%");
  }
}

// ==== CEK JADWAL ====
void checkScheduledBlower() {
  if (!blowerAutoEnable) return;

  DateTime now = rtc.now();
  if (now.hour() == autoBlowerHour && now.minute() == autoBlowerMinute &&
      !blowerAutoTriggered && now.second() < 5) {
    Blynk.logEvent("blower_otomatis_on", "Blower otomatis dimulai");
    digitalWrite(RELAY_PIN, HIGH);
    myServo.write(90);
    blowerActive = true;
    blowerAutoStart = millis();
    blowerAutoTriggered = true;
  }
}

// ==== CEK BLOWER WAKTU ====
void checkBlowerTimeout() {
  // Notifikasi blower manual aktif 5 menit
  if (blowerActive && (millis() - blowerManualStart >= 300000)) {
    Blynk.logEvent("pakan_5menit", "Pemberian pakan sudah berlangsung 5 menit");
    blowerManualStart = millis(); // reset
  }

  if (blowerAutoTriggered && blowerActive &&
      (millis() - blowerAutoStart >= blowerAutoDuration)) {
    digitalWrite(RELAY_PIN, LOW);
    myServo.write(0);
    blowerActive = false;
    blowerAutoTriggered = false;
    Blynk.logEvent("blower_otomatis_off", "Blower otomatis selesai");
  }
}

// ==== UPDATE LCD ====
void updateLCD() {
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);
  lcd.print(String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute()));
  lcd.print(" PH:" + String(lastPH, 1));

  lcd.setCursor(0, 1);
  if (blowerActive) {
    lcd.print("Blower:ON ");
  } else {
    lcd.print("Blower:OFF");
  }
  lcd.print(" Pkn:" + String(lastKapasitas) + "% ");
}

// ==== SETUP ====
void setup() {
  Serial.begin(115200);

  pinMode(PH_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW);
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Pakan");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  if (!rtc.begin()) {
  Serial.println("RTC tidak ditemukan! Lanjut tanpa RTC.");
} else {
  Serial.println("RTC berhasil ditemukan.");
}


  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(10000L, sendPH);
  timer.setInterval(30000L, sendPakanLevel);
  timer.setInterval(1000L, checkScheduledBlower);
  timer.setInterval(1000L, checkBlowerTimeout);
  timer.setInterval(2000L, updateLCD); // Tambahkan update LCD
}

// ==== LOOP ====
void loop() {
  Blynk.run();
  timer.run();
}

 // Berikut adalah daftar pin yang digunakan dalam kode program tersebut:

// PH_PIN (Pin 34): Pin analog untuk membaca sensor pH. biru

// RELAY_PIN (Pin 19): Pin digital untuk mengendalikan relay (blower). tali kuning

// SERVO_PIN (Pin 18): Pin digital untuk mengendalikan motor servo. tali oren

// TRIG_PIN (Pin 26): Pin digital untuk mengirimkan sinyal trigger pada sensor ultrasonik. hijau 

// ECHO_PIN (Pin 25): Pin digital untuk menerima sinyal echo dari sensor ultrasonik. ungu


// SDA: Pin 21 tali hijau

// SCL: Pin 22 tali biru