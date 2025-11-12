// Tidak perlu WiFi.h jika tidak digunakan
// #include <WiFi.h> 

const int ph_Pin = 32; // Pin input sensor pH (gunakan pin ADC ESP32)

// Fungsi rata-rata pembacaan ADC
float readAverage(int pin, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(10);
  }
  return sum / (float)samples;
}

void setup() {
  Serial.begin(115200);
  pinMode(ph_Pin, INPUT);
}

void loop() {
  int samples = 10;
  int nilai_analog_pH = readAverage(ph_Pin, samples);

  // Konversi nilai ADC (0 - 4095) ke Tegangan (0 - 3.3V)
  float tegangan_pH = (nilai_analog_pH * 3.3) / 4095.0;

  // Kalibrasi berdasarkan dua titik: pH 7 = 2.490V dan pH 4 = 3.078V
  float pH = -5.102 * tegangan_pH + 19.708;

  // Tampilkan hasil ke Serial Monitor
  Serial.print("Nilai ADC pH   : ");
  Serial.println(nilai_analog_pH);
  Serial.print("Tegangan pH    : ");
  Serial.print(tegangan_pH, 3);
  Serial.println(" V");
  Serial.print("Nilai pH cairan: ");
  Serial.println(pH, 3);
  Serial.println("---------------------------");

  delay(1000); // delay antar pembacaan
}
