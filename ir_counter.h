#ifndef IR_COUNTER_H
#define IR_COUNTER_H

#define IR_PIN 32   // IR aktif LOW

int irCount = 0;
bool objectDetected = false;
unsigned long lastChangeTime = 0;
const unsigned long debounceDelay = 100; // ms

void setupIR() {
  pinMode(IR_PIN, INPUT_PULLUP);
  Serial.println("IR sensor siap...");
}

void updateIR() {
  int state = digitalRead(IR_PIN);
  unsigned long now = millis();
  static int lastState = HIGH;

  if (state != lastState) {
    lastChangeTime = now;
    lastState = state;
  }

  if ((now - lastChangeTime) > debounceDelay) {
    if (state == LOW && !objectDetected) {
      objectDetected = true;
      irCount++;
      Serial.printf("🔻 Objek TERDETEKSI | Hitungan = %d\n", irCount);
    } else if (state == HIGH && objectDetected) {
      objectDetected = false;
      Serial.println("🔺 Objek LEPAS");
    }
  }
}

int getIRCount() {
  return irCount;
}

void resetIRCount() {
  irCount = 0;
}

#endif
