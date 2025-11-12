// Konfigurasi Blynk (gunakan template dan auth token dari dashboard Blynk)
#define BLYNK_TEMPLATE_ID "TMPL6tlPOaJPd"
#define BLYNK_TEMPLATE_NAME "Pakan Lele Otomatis dan Monitoring Kualitas Air"
#define BLYNK_AUTH_TOKEN "Ic2Jjz9IxTzDTONIIxtLOg0FXn5cPN8X"

// Library yang diperlukan
#include <WiFi.h>                 // Untuk koneksi WiFi
#include <WiFiClient.h>          // Untuk koneksi TCP/IP
#include <BlynkSimpleEsp32.h>    // Library Blynk khusus ESP32
#include <RTClib.h>              // Library untuk Real-Time Clock
#include <ESP32Servo.h>          // Library kontrol Servo
#include <LiquidCrystal_I2C.h>   // Library LCD I2C

// Data koneksi WiFi
char ssid[] = "TestingPA";
char pass[] = "inikelompok5";

// Definisi pin yang digunakan
#define PH_PIN 34        // Pin ADC untuk sensor pH
#define RELAY_PIN 19     // Relay blower pakan
#define SERVO_PIN 18     // Servo pengontrol pakan
#define TRIG_PIN 26      // Trigger sensor ultrasonik
#define ECHO_PIN 25      // Echo sensor ultrasonik

// Deklarasi objek perangkat keras
RTC_DS3231 rtc;              // RTC module
Servo myServo;               // Servo object
BlynkTimer timer;            // Timer Blynk
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C 16x2

// Status sistem
bool manualFeeding = false;
bool autoFeeding = false;
bool autoFeedingEnabled = false;

unsigned long manualStartTime = 0;
unsigned long autoStartTime = 0;
unsigned long manualDuration = 0;
unsigned long autoDuration = 0;

int lcdPage = 1;          // Untuk tampilan halaman LCD
float lastPH = 0.0;       // Menyimpan pH terakhir
int lastKapasitas = 0;    // Menyimpan kapasitas pakan terakhir

bool lowPakanSent = false;     // Flag notifikasi pakan
bool abnormalPhSent = false;   // Flag notifikasi pH

// Jadwal pemberian pakan otomatis
int scheduleHours[] = {8, 15, 19};     // Jam pemberian
int scheduleMinutes[] = {30, 35, 30};  // Menit pemberian
bool scheduleDone[3] = {false, false, false}; // Flag jadwal sudah dilaksanakan

// Fungsi membaca sensor pH dan kirim ke Blynk
void sendPH() {
  int sensorValue = analogRead(PH_PIN);                         // Baca data analog
  float voltage = sensorValue * (3.3 / 4095.0);                 // Konversi ke tegangan
  float ph = 7 + ((2.5 - voltage) / 0.18);                      // Konversi ke nilai pH
  lastPH = ph;

  Serial.printf("[pH Sensor] Raw: %d | Voltage: %.2f V | pH: %.2f\n", sensorValue, voltage, ph);
  Blynk.virtualWrite(V10, ph);                                  // Kirim ke Blynk (V10)

  if ((ph < 6.5 || ph > 8.5) && !abnormalPhSent) {
    Blynk.logEvent("ph_abnormal", "pH air tidak normal!");     // Kirim notifikasi
    abnormalPhSent = true;
  } else if (ph >= 6.5 && ph <= 8.5) {
    abnormalPhSent = false;
  }
}

// Fungsi membaca level pakan dengan sensor ultrasonik
void sendPakanLevel() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);               // Baca durasi pantulan

  if (duration == 0) {
    Serial.println("[PakanLevel] Tidak ada pantulan (timeout)");
    return;
  }

  float distance = duration * 0.034 / 2.0;                      // Konversi ke cm
  Serial.printf("[PakanLevel] Jarak: %.2f cm\n", distance);

  // Estimasi kapasitas pakan berdasarkan jarak
  if (distance > 35) lastKapasitas = 0;
  else if (distance > 30) lastKapasitas = 10;
  else if (distance > 25) lastKapasitas = 30;
  else if (distance > 20) lastKapasitas = 50;
  else if (distance > 10) lastKapasitas = 70;
  else lastKapasitas = 90;

  Blynk.virtualWrite(V11, lastKapasitas);                       // Kirim ke Blynk (V11)
  Serial.printf("[PakanLevel] Kapasitas: %d%%\n", lastKapasitas);

  if (lastKapasitas <= 10 && !lowPakanSent) {
    Blynk.logEvent("pakan_habis", "Pakan hampir habis!");
    lowPakanSent = true;
  } else if (lastKapasitas > 10) {
    lowPakanSent = false;
  }
}

// Fungsi untuk memulai pemberian pakan
void startFeeding(bool isManual) {
  if ((isManual && manualFeeding) || (!isManual && autoFeeding)) return;

  digitalWrite(RELAY_PIN, LOW);      // Aktifkan blower
  myServo.write(0);                  // Buka servo (posisi terbuka)
  delay(500);

  if (isManual) {
    manualFeeding = true;
    manualStartTime = millis();
    Blynk.logEvent("blower_on", "Blower dinyalakan secara manual.");
  } else {
    autoFeeding = true;
    autoStartTime = millis();
    Blynk.logEvent("blower_on", "Blower dinyalakan oleh sistem otomatis.");
  }

  Serial.printf("Feeding dimulai (%s)\n", isManual ? "Manual" : "Otomatis");
}

// Fungsi menghentikan feeding
void stopFeeding(bool isManual) {
  myServo.write(90);             // Tutup servo
  delay(500);
  digitalWrite(RELAY_PIN, HIGH); // Matikan blower

  if (isManual) {
    manualFeeding = false;
    Blynk.logEvent("blower_off", "Blower dimatikan secara manual.");
  } else {
    autoFeeding = false;
    Blynk.logEvent("blower_off", "Blower dimatikan secara otomatis.");
  }

  Serial.printf("Feeding dihentikan (%s)\n", isManual ? "Manual" : "Otomatis");
}

// Fungsi pengecekan jadwal otomatis
void checkAutoSchedule() {
  if (!autoFeedingEnabled || autoDuration == 0) return;

  DateTime now = rtc.now(); // Baca waktu sekarang

  for (int i = 0; i < 3; i++) {
    if (now.hour() == scheduleHours[i] &&
        now.minute() == scheduleMinutes[i] &&
        now.second() < 5 &&
        !scheduleDone[i]) {
      Serial.printf("Menjalankan jadwal feeding ke-%d\n", i + 1);
      startFeeding(false); // Mulai feeding otomatis
      scheduleDone[i] = true;
    }
  }

  // Reset jadwal setiap tengah malam
  if (now.hour() == 0 && now.minute() == 0 && now.second() < 5) {
    for (int i = 0; i < 3; i++) scheduleDone[i] = false;
    Serial.println("Reset jadwal harian (00:00)");
  }
}

// Fungsi pengecekan durasi feeding manual/otomatis
void checkFeedingDuration() {
  if (manualFeeding && millis() - manualStartTime >= manualDuration) stopFeeding(true);
  if (autoFeeding && millis() - autoStartTime >= autoDuration) stopFeeding(false);
}

// Fungsi memperbarui LCD setiap 2 detik
void updateLCD() {
  lcd.clear();
  if (lcdPage == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Jadwal: 08 17 19");
    lcd.setCursor(0, 1);
    lcd.printf("Pkn:%d%% PH:%.1f", lastKapasitas, lastPH);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Manual:");
    lcd.print(manualFeeding ? "ON " : "OFF");
    lcd.print(" ");
    lcd.print(manualDuration / 60000);
    lcd.print("m");

    lcd.setCursor(0, 1);
    lcd.print("Auto:");
    lcd.print(autoFeedingEnabled ? "ON " : "OFF");
    lcd.print(" ");
    lcd.print(autoDuration / 60000);
    lcd.print("m");
  }
}

// Cetak waktu dari RTC ke serial
void printRTCTime() {
  DateTime now = rtc.now();
  Serial.printf("[RTC] %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
}

// Atur RTC jika pertama kali atau reset
void setRTCOnce() {
  if (rtc.lostPower() || rtc.now().year() < 2024) {
    Serial.println("[RTC] Mengatur waktu default...");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
}

// HANDLER DARI APLIKASI BLYNK

// Tombol ON/OFF blower manual
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1 && !manualFeeding) startFeeding(true);
  else if (value == 0 && manualFeeding) stopFeeding(true);
}

// Slider durasi blower manual (dalam menit)
BLYNK_WRITE(V1) {
  manualDuration = param.asInt() * 60000;
}

// Tombol mengaktifkan mode auto feeding
BLYNK_WRITE(V2) {
  autoFeedingEnabled = param.asInt();
  Serial.printf("AutoFeedingEnabled = %d\n", autoFeedingEnabled);
}

// Slider durasi blower otomatis (dalam menit)
BLYNK_WRITE(V3) {
  autoDuration = param.asInt() * 60000;
  Serial.printf("AutoDuration (ms) = %lu\n", autoDuration);
}

// Tombol untuk memilih halaman LCD
BLYNK_WRITE(V4) {
  lcdPage = param.asInt();
}

// SETUP AWAL SAAT PERANGKAT DINYALAKAN
void setup() {
  Serial.begin(115200); // Mulai serial monitor

  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, HIGH);
  pinMode(PH_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myServo.write(90); // Posisi awal servo (tutup)
  delay(300);
  myServo.attach(SERVO_PIN); // Attach servo

  lcd.init(); lcd.backlight(); // Inisialisasi LCD

  if (!rtc.begin()) {
    Serial.println("[ERROR] RTC tidak ditemukan!");
  } else {
    setRTCOnce();
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // Hubungkan ke Blynk

  // Timer untuk menjalankan fungsi berkala
  timer.setInterval(10000L, sendPH);
  timer.setInterval(2000L, sendPakanLevel);
  timer.setInterval(1000L, checkAutoSchedule);
  timer.setInterval(1000L, checkFeedingDuration);
  timer.setInterval(2000L, updateLCD);
  timer.setInterval(10000L, printRTCTime);
}

// LOOP UTAMA
void loop() {
  Blynk.run();  // Jalankan proses Blynk
  timer.run();  // Jalankan semua fungsi timer
}
