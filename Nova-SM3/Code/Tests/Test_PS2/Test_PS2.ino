/*
 *   Nova SM3 - Bench Test: PS2 wireless controller
 *   TARGET BOARD: Teensy 4.0   (master)
 *
 *   Pins match Nova-SM3_teensy-v5.0:
 *     PS2_DAT = 7   PS2_CMD = 8   PS2_SEL = 9   PS2_CLK = 10
 *
 *   Uses the Teensy PS2X fork the main sketch depends on:
 *     https://github.com/KurtE/Arduino-PS2X   (vendored in ../Arduino Libraries/PS2X_lib)
 *   In that fork config_gamepad() returns 0 on SUCCESS, which is what the
 *   main sketch checks - this test matches that convention.
 *
 *   *** WIRING NOTE ***
 *   The PS2 receiver is a 3.3V part. The Teensy 4.0 is ALSO 3.3V and is NOT
 *   5V tolerant - do not feed the receiver 5V and then wire DAT back to the
 *   Teensy. If this test finds no controller, check that first.
 *
 *   PASS CRITERIA:
 *     - "controller found" at boot (retries for 10s if not)
 *     - onboard orange LED (pin 13) solid = connected, blinking = searching
 *     - every button press prints exactly once
 *     - both sticks read ~128 centred, 0..255 at full deflection
 */

#include <PS2X_lib.h>

#define PS2_DAT 7
#define PS2_CMD 8
#define PS2_SEL 9
#define PS2_CLK 10
#define LED_PIN 13

PS2X ps2x;
byte connected = 0;

unsigned long lastRead = 0;
const unsigned int readInterval = 50;     // matches ps2Interval in main sketch

unsigned long lastStick = 0;
const unsigned int stickInterval = 300;

// analog centre tolerance - sticks rarely sit at exactly 128
const byte deadzone = 8;

struct Btn { uint16_t mask; const char* name; };
const Btn buttons[] = {
  {PSB_START,    "START"},   {PSB_SELECT,   "SELECT"},
  {PSB_PAD_UP,   "UP"},      {PSB_PAD_DOWN, "DOWN"},
  {PSB_PAD_LEFT, "LEFT"},    {PSB_PAD_RIGHT,"RIGHT"},
  {PSB_TRIANGLE, "TRIANGLE"},{PSB_CROSS,    "CROSS"},
  {PSB_CIRCLE,   "CIRCLE"},  {PSB_SQUARE,   "SQUARE"},
  {PSB_L1,       "L1"},      {PSB_R1,       "R1"},
  {PSB_L2,       "L2"},      {PSB_R2,       "R2"},
  {PSB_L3,       "L3"},      {PSB_R3,       "R3"},
};
const byte numButtons = sizeof(buttons) / sizeof(buttons[0]);

void tryConnect() {
  Serial.println(F("looking for controller..."));
  for (byte attempt = 1; attempt <= 10; attempt++) {
    int err = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
    if (err == 0) {
      connected = 1;
      digitalWrite(LED_PIN, HIGH);     // solid = connected, incl. after a retry
      Serial.print(F("controller found on attempt "));
      Serial.println(attempt);
      return;
    }
    Serial.print(F("  attempt ")); Serial.print(attempt);
    Serial.print(F(" failed (err ")); Serial.print(err); Serial.println(F(")"));
    delay(1000);
  }
  Serial.println(F("\nNO CONTROLLER."));
  Serial.println(F("  - is the receiver paired (solid LED, not blinking)?"));
  Serial.println(F("  - check DAT=7 CMD=8 SEL=9 CLK=10, and 3.3V not 5V"));
  Serial.println(F("  - some clones need a power cycle of the receiver"));
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println(F("\n=== Nova SM3 :: PS2 controller test (Teensy 4.0) ==="));
  Serial.print(F("DAT=")); Serial.print(PS2_DAT);
  Serial.print(F("  CMD=")); Serial.print(PS2_CMD);
  Serial.print(F("  SEL=")); Serial.print(PS2_SEL);
  Serial.print(F("  CLK=")); Serial.println(PS2_CLK);
  Serial.println();

  delay(300);
  tryConnect();

  if (connected) {
    Serial.println(F("\nPress buttons / move sticks.\n"));
  }
}

void loop() {
  if (!connected) {
    // slow blink + periodic retry, so you can hot-plug the receiver
    digitalWrite(LED_PIN, (millis() / 500) % 2);
    if (millis() - lastRead > 5000) {
      lastRead = millis();
      tryConnect();
      if (connected) Serial.println(F("\nPress buttons / move sticks.\n"));
    }
    return;
  }

  if (millis() - lastRead < readInterval) return;
  lastRead = millis();

  ps2x.read_gamepad(false, false);

  // --- buttons: NewButtonState fires once per press ---
  for (byte i = 0; i < numButtons; i++) {
    if (ps2x.ButtonPressed(buttons[i].mask)) {
      Serial.print(F("DOWN  "));
      Serial.print(buttons[i].name);
      if (buttons[i].mask == PSB_L2 || buttons[i].mask == PSB_R2 ||
          buttons[i].mask == PSB_L1 || buttons[i].mask == PSB_R1) {
        Serial.print(F("   analog:"));
        Serial.print(ps2x.Analog(buttons[i].mask));
      }
      Serial.println();
    }
    if (ps2x.ButtonReleased(buttons[i].mask)) {
      Serial.print(F("  up  "));
      Serial.println(buttons[i].name);
    }
  }

  // --- sticks: only report when off-centre, else the log floods ---
  byte lx = ps2x.Analog(PSS_LX), ly = ps2x.Analog(PSS_LY);
  byte rx = ps2x.Analog(PSS_RX), ry = ps2x.Analog(PSS_RY);

  byte moved = (abs((int)lx - 128) > deadzone) || (abs((int)ly - 128) > deadzone) ||
               (abs((int)rx - 128) > deadzone) || (abs((int)ry - 128) > deadzone);

  if (moved && millis() - lastStick > 100) {
    lastStick = millis();
    Serial.print(F("stick  LX:")); Serial.print(lx);
    Serial.print(F(" LY:"));       Serial.print(ly);
    Serial.print(F("   RX:"));     Serial.print(rx);
    Serial.print(F(" RY:"));       Serial.println(ry);
  }

  // heartbeat with centred values, proves polling is still alive
  if (!moved && millis() - lastStick > stickInterval * 10) {
    lastStick = millis();
    Serial.print(F("(idle, sticks centred  LX:")); Serial.print(lx);
    Serial.print(F(" LY:")); Serial.print(ly);
    Serial.print(F(" RX:")); Serial.print(rx);
    Serial.print(F(" RY:")); Serial.println(ry);
  }
}
