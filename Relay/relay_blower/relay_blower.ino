const int relay1 = 19;    // IN1
const int relay2 = 18;   // IN2

void setup() {
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  digitalWrite(relay1, LOW);  // Nyalakan relay 1
  digitalWrite(relay2, LOW);  // Nyalakan relay 2

  Serial.begin(9600);
  Serial.println("Relay 1 dan 2 aktif");
}

void loop() {
  // Tidak ada apa-apa di loop
}
