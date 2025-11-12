#include <ESP32Servo.h>

Servo myServo;

int servoPin = 13;

void setup() {
  Serial.begin(115200);
  myServo.attach(servoPin); // Attach servo to GPIO 13
  Serial.println("Servo test dimulai...");
}

void loop() {
  // Gerakkan servo dari 0 ke 90 derajat
  myServo.write(90);
  Serial.println("Posisi: 90");
  delay(1000); // Tahan di posisi 90 derajat selama 1 detik

  // Gerakkan kembali ke 0 derajat
  myServo.write(0);
  Serial.println("Posisi: 0");
  delay(1000); // Tahan di posisi 0 derajat selama 1 detik
}
