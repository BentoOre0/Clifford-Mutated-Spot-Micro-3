/*
 *   Nova SM3 - Bench Test: HC-SR04 ultrasonic sensors (2)
 *   TARGET BOARD: Arduino Nano   (slave)
 *
 *   Pins match Nova-SM3_nano-v5.1:
 *     LEFT   TRIG = 7   ECHO = 6
 *     RIGHT  TRIG = 5   ECHO = 4
 *     MAX_DISTANCE = 300 cm
 *
 *   Uses NewPing (vendored in ../Arduino Libraries/NewPing), same as the
 *   slave sketch, so a pass here means the slave's readings should work too.
 *
 *   *** WIRING NOTE ***
 *   HC-SR04 ECHO idles at 5V. That is fine on a Nano (5V logic). If you ever
 *   move these to the Teensy, ECHO needs a divider - the Teensy is not 5V
 *   tolerant. Also give these sensors a solid 5V rail; brown-outs from servo
 *   current are a classic cause of erratic readings.
 *
 *   PASS CRITERIA:
 *     - a hand at ~20cm reads ~20 on the correct side
 *     - 0 means "no echo" (out of range OR disconnected) - check both
 *     - readings should be stable +/-1cm against a flat wall
 */

#include <NewPing.h>

#define SONAR_NUM    2
#define L_TRIGPIN    7
#define L_ECHOPIN    6
#define R_TRIGPIN    5
#define R_ECHOPIN    4
#define MAX_DISTANCE 300

NewPing sonar[SONAR_NUM] = {
  NewPing(L_TRIGPIN, L_ECHOPIN, MAX_DISTANCE),
  NewPing(R_TRIGPIN, R_ECHOPIN, MAX_DISTANCE),
};

// the slave polls at 250ms; 100ms here gives a livelier bench feel.
// don't go below ~35ms or the two sensors will hear each other's pings.
const unsigned int pingInterval = 100;
unsigned long lastPing = 0;

// rolling stats so you can spot dropouts rather than eyeball a scrolling list
int  lastL = 0, lastR = 0;
long readsL = 0, readsR = 0, zerosL = 0, zerosR = 0;
unsigned long lastStats = 0;

void bar(int cm) {
  // crude visual: one '#' per 5cm, capped
  int n = cm / 5;
  if (n > 30) n = 30;
  for (int i = 0; i < n; i++) Serial.print('#');
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println(F("\n=== Nova SM3 :: ultrasonic test (Arduino Nano) ==="));
  Serial.print(F("LEFT  trig=")); Serial.print(L_TRIGPIN);
  Serial.print(F(" echo="));      Serial.println(L_ECHOPIN);
  Serial.print(F("RIGHT trig=")); Serial.print(R_TRIGPIN);
  Serial.print(F(" echo="));      Serial.println(R_ECHOPIN);
  Serial.println(F("\n0 = no echo (out of range or not connected)\n"));
}

void loop() {
  if (millis() - lastPing < pingInterval) return;
  lastPing = millis();

  // ping one at a time with a gap, so the two don't cross-talk
  int l = sonar[0].ping_cm();
  delay(30);
  int r = sonar[1].ping_cm();

  readsL++; if (l == 0) zerosL++;
  readsR++; if (r == 0) zerosR++;

  Serial.print(F("L:"));
  if (l < 100) Serial.print(' ');
  if (l < 10)  Serial.print(' ');
  Serial.print(l);
  Serial.print(F("cm  R:"));
  if (r < 100) Serial.print(' ');
  if (r < 10)  Serial.print(' ');
  Serial.print(r);
  Serial.print(F("cm  |"));

  // show whichever side is closer, as a bar
  int closer = (l && r) ? min(l, r) : (l ? l : r);
  bar(closer);
  Serial.println();

  lastL = l; lastR = r;

  // dropout summary every 10s
  if (millis() - lastStats > 10000) {
    lastStats = millis();
    Serial.print(F("\n-- no-echo rate  L: "));
    Serial.print((zerosL * 100) / max(readsL, 1L));
    Serial.print(F("%   R: "));
    Serial.print((zerosR * 100) / max(readsR, 1L));
    Serial.println(F("%   (high % pointing at open air is normal)\n"));
    readsL = readsR = zerosL = zerosR = 0;
  }
}
