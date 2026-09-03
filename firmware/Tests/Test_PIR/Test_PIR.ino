/*
 *   Nova SM3 - Bench Test: PIR motion sensors (3)
 *   TARGET BOARD: Teensy 4.0   (master)
 *
 *   Pins match Nova-SM3_teensy-v5.0:
 *     PIR_FRONT = 4    PIR_LEFT = 5    PIR_RIGHT = 6
 *
 *   NOTE ON PIR MODULES: most HC-SR501-style boards need 30-60s to settle
 *   after power-up and will report garbage until they do. This sketch runs a
 *   warm-up countdown first - don't judge the sensors before it finishes.
 *
 *   Each module also has two trim pots (sensitivity + hold time) and a
 *   retrigger jumper; if a channel latches HIGH forever, turn the hold-time
 *   pot fully counter-clockwise.
 *
 *   PASS CRITERIA:
 *     - all three read LOW with the room still (after warm-up)
 *     - waving in front of one sensor trips ONLY that channel
 */

#define PIR_FRONT 4
#define PIR_LEFT  5
#define PIR_RIGHT 6
#define LED_PIN   13

const byte  pirPin[3]  = {PIR_FRONT, PIR_LEFT, PIR_RIGHT};
const char* pirName[3] = {"FRONT", "LEFT ", "RIGHT"};

int  lastState[3]  = {LOW, LOW, LOW};
long tripCount[3]  = {0, 0, 0};

const unsigned int warmupSeconds = 45;
unsigned long lastPrint = 0;
const unsigned int printInterval = 2000;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  for (byte i = 0; i < 3; i++) pinMode(pirPin[i], INPUT);

  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println(F("\n=== Nova SM3 :: PIR test (Teensy 4.0) ==="));
  Serial.print(F("FRONT=")); Serial.print(PIR_FRONT);
  Serial.print(F("  LEFT=")); Serial.print(PIR_LEFT);
  Serial.print(F("  RIGHT=")); Serial.println(PIR_RIGHT);

  Serial.print(F("\nWarming up sensors, keep still: "));
  for (unsigned int s = warmupSeconds; s > 0; s--) {
    if (s % 5 == 0) { Serial.print(s); Serial.print(F(" ")); }
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(1000);
  }
  digitalWrite(LED_PIN, LOW);

  Serial.println(F("\n\nReady. Wave a hand at each sensor in turn.\n"));
  for (byte i = 0; i < 3; i++) lastState[i] = digitalRead(pirPin[i]);
}

void loop() {
  byte anyHigh = 0;

  for (byte i = 0; i < 3; i++) {
    int s = digitalRead(pirPin[i]);
    if (s) anyHigh = 1;

    if (s != lastState[i]) {
      lastState[i] = s;
      if (s == HIGH) tripCount[i]++;

      Serial.print(F("["));
      Serial.print(millis() / 1000);
      Serial.print(F("s] "));
      Serial.print(pirName[i]);
      Serial.print(s == HIGH ? F("  *** MOTION ***") : F("  ... clear"));
      if (s == HIGH) {
        Serial.print(F("   (trips: "));
        Serial.print(tripCount[i]);
        Serial.print(F(")"));
      }
      Serial.println();
    }
  }

  digitalWrite(LED_PIN, anyHigh);

  // periodic idle line, so you can tell the sketch is alive vs. hung
  if (millis() - lastPrint > printInterval) {
    lastPrint = millis();
    if (!anyHigh) {
      Serial.print(F("idle   F:"));
      Serial.print(lastState[0]);
      Serial.print(F(" L:"));
      Serial.print(lastState[1]);
      Serial.print(F(" R:"));
      Serial.print(lastState[2]);
      Serial.print(F("   trips F/L/R: "));
      Serial.print(tripCount[0]); Serial.print(F("/"));
      Serial.print(tripCount[1]); Serial.print(F("/"));
      Serial.println(tripCount[2]);
    }
  }
}
