void setup() {
    Serial.begin(115200);
}

void loop() {
    int sensorValue = analogRead(34); // Baca dari pin 34
    Serial.println(sensorValue); // Cek apakah ada perubahan nilai
    delay(1000);
}
