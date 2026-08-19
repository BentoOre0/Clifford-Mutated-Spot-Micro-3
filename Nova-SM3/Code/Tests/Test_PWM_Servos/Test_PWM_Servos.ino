/*
 *   Nova SM3 - Bench Test: ONE servo at a time, driven from the PS2 controller
 *   TARGET BOARD: Teensy 4.0   (master)
 *
 *   WHAT THIS SKETCH IS FOR
 *   -----------------------
 *   You have 12 servos wired into a PCA9685 driver board. This sketch moves
 *   exactly ONE of them at a time, so you can confirm two things:
 *
 *     1. The servo physically works and moves smoothly.
 *     2. The servo is plugged into the channel the firmware THINKS it is.
 *
 *   Point 2 is the one that actually catches bugs. If you swap two servo
 *   cables on the driver board, nothing errors - the robot just moves wrong.
 *   The only way to find it is to command one joint and watch which joint
 *   actually moves. That is the whole point of this test.
 *
 *   HOW YOU DRIVE IT
 *   ----------------
 *   With the PS2 controller. There is one way to move the servo:
 *
 *     HOLD  D-pad UP     - rotate one way  (PWM value counts up)
 *     HOLD  D-pad DOWN   - rotate the other way (PWM value counts down)
 *     TAP   D-pad RIGHT  - switch to the NEXT servo
 *     TAP   D-pad LEFT   - switch to the PREVIOUS servo
 *     TAP   CROSS (X)    - send the current servo back to its home position
 *     TAP   START        - ENABLE servo power (nothing moves until you do this)
 *     TAP   SELECT       - DISABLE servo power (everything goes limp)
 *
 *   The serial monitor prints what is happening, and also accepts a few
 *   backup keys in case the controller drops out mid-test - see loop().
 *
 *   *** SAFETY - READ THIS ***
 *   - PUT THE ROBOT ON A STAND with the legs hanging free.
 *   - Servo power is DISABLED at boot. Nothing can move until you press START.
 *   - Pressing SELECT cuts power instantly. If the robot is standing on its
 *     own legs when you do that, it WILL collapse. Stand. Always.
 *   - Movement is clamped to servoLimit[] so a held button cannot drive a
 *     joint past its mechanical stop and strip a gear.
 *
 *   WIRING
 *   ------
 *     PCA9685  -> Teensy default i2c bus (Wire: SDA 18, SCL 19), address 0x40
 *     OE_PIN   -> Teensy pin 3   (output enable, see note below)
 *     PS2 recv -> DAT 7, CMD 8, SEL 9, CLK 10
 *     Servo power: separate 5-6V supply into the PCA9685 screw terminal,
 *                  with its ground tied to Teensy ground. Do NOT try to run
 *                  12 servos off the Teensy's USB 5V.
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <PS2X_lib.h>


/* =====================================================================
   SECTION 1: CONFIGURATION - the numbers you might want to change
   ===================================================================== */

// Which servo does the test start on? This is an index 0-11 into the arrays
// in SECTION 2 below. 0 = RFC = Right Front Coxa. You can also change servo
// live with D-pad LEFT/RIGHT, so this is only the starting point.
#define START_SERVO 0

// How many PWM ticks to move per poll while a D-pad button is held.
// The controller is polled every PS2_INTERVAL ms, so the actual speed is:
//     JOG_STEP ticks x (1000 / PS2_INTERVAL) polls per second
// With the defaults below that is 2 x 20 = 40 ticks per second, which is a
// slow, safe crawl. Raise JOG_STEP to move faster. Start slow.
#define JOG_STEP     2
#define PS2_INTERVAL 50    // ms between controller reads

// Hardware pins
#define OE_PIN   3         // PCA9685 output enable
#define LED_PIN  13        // Teensy onboard orange LED
#define PS2_DAT  7
#define PS2_CMD  8
#define PS2_SEL  9
#define PS2_CLK  10

// PCA9685 timing. CAUTION: these match the calibrated main sketch. The
// servoHome/servoLimit numbers below are only correct AT THESE FREQUENCIES.
// Change them and every calibrated position in the robot becomes wrong.
#define SERVO_FREQ 60
#define OSCIL_FREQ 25000000

Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
PS2X ps2x;


/* =====================================================================
   SECTION 2: CALIBRATION DATA - copied verbatim from NovaServos.h

   These numbers describe ONE physical robot. They are measured on the
   bench, not calculated. Do not "tidy" them.
   ===================================================================== */

#define TOTAL_SERVOS 12

// Which PCA9685 channel each servo is plugged into.
// The first number is the driver board ID (always 1, there is only one
// board). The second is the channel on that board.
//
// NOTE the gaps: channels 3, 7, 11 and 15 are deliberately skipped, so the
// servo index and the channel number are NOT the same. Servo index 3 (LFC)
// lives on channel 4. This is the single most confusing thing in the file,
// which is why the printouts always show both.
int servoSetup[TOTAL_SERVOS][2] = {       // driver ID, channel number
  {1,0},  {1,1},  {1,2},                  // RFx - right front
  {1,4},  {1,5},  {1,6},                  // LFx - left front
  {1,8},  {1,9},  {1,10},                 // RRx - right rear
  {1,12}, {1,13}, {1,14},                 // LRx - left rear
};

// The "standing straight" position for each servo, as a raw PWM tick count.
float servoHome[TOTAL_SERVOS] = {
  352, 280, 510,                          // RFx
  364, 442, 225,                          // LFx
  367, 331, 423,                          // RRx
  364, 351, 213,                          // LRx
};

// How far each servo is allowed to travel, as {min, max}.
//
// IMPORTANT: these pairs are min/max in the LEG's direction of travel, not
// in numeric order. Look at LFF: {537, 207}. The first number is BIGGER than
// the second. That is not a typo - the left legs are mirror images of the
// right ones, so their servos count the other way.
//
// This means you can never just write `if (pos > limit[1])`. You have to
// sort the pair first. clampToLimit() below does exactly that.
float servoLimit[TOTAL_SERVOS][2] = {     // min, max in leg direction
  {314, 434}, {185, 515}, {365, 607},     // RFx
  {402, 282}, {537, 207}, {370, 128},     // LFx
  {329, 449}, {236, 566}, {278, 520},     // RRx
  {402, 282}, {446, 116}, {358, 116},     // LRx
};

// Short names, and the plain-English joint each one drives. When you command
// a servo, THIS is the joint you should see move. If a different one moves,
// the cable is in the wrong channel.
const char* servoName[TOTAL_SERVOS] = {
  "RFC", "RFF", "RFT",  "LFC", "LFF", "LFT",
  "RRC", "RRF", "RRT",  "LRC", "LRF", "LRT",
};
const char* servoJoint[TOTAL_SERVOS] = {
  "right front COXA (hip swing)",  "right front FEMUR (thigh)",  "right front TIBIA (shin)",
  "left front COXA (hip swing)",   "left front FEMUR (thigh)",   "left front TIBIA (shin)",
  "right rear COXA (hip swing)",   "right rear FEMUR (thigh)",   "right rear TIBIA (shin)",
  "left rear COXA (hip swing)",    "left rear FEMUR (thigh)",    "left rear TIBIA (shin)",
};


/* =====================================================================
   SECTION 3: STATE - the handful of variables that change as it runs
   ===================================================================== */

float servoPos[TOTAL_SERVOS];   // where we believe each servo currently is
int   sel       = START_SERVO;  // which servo the D-pad is driving
byte  outputOn  = 0;            // 1 once START has been pressed
byte  ps2Ready  = 0;            // 1 if the controller handshake succeeded

unsigned long lastPS2Read = 0;  // for the non-blocking poll timer


/* =====================================================================
   SECTION 4: HELPERS
   ===================================================================== */

// Squeeze a requested position into the servo's safe travel range.
// Because servoLimit pairs can be high-then-low (see SECTION 2), we work out
// the numeric low and high first, then clamp against those.
float clampToLimit(int i, float pos) {
  float lo = min(servoLimit[i][0], servoLimit[i][1]);
  float hi = max(servoLimit[i][0], servoLimit[i][1]);
  if (pos < lo) return lo;
  if (pos > hi) return hi;
  return pos;
}

// Send one servo to a position. This is the ONLY function in the sketch that
// actually talks to the servo driver - everything else works out what to ask
// for and then calls this.
void moveServo(int i, float pos) {
  float clamped = clampToLimit(i, pos);
  servoPos[i] = clamped;

  // setPWM(channel, on_tick, off_tick). The PCA9685 counts 0..4095 over each
  // 60Hz cycle. We always switch on at tick 0, and switch off at `clamped` -
  // so `clamped` IS the pulse width, and that is what sets the servo angle.
  pwm1.setPWM(servoSetup[i][1], 0, (int)clamped);
}

// Print which servo is selected and what it should move.
void announceServo() {
  Serial.println();
  Serial.print(F("SELECTED: "));   Serial.print(servoName[sel]);
  Serial.print(F("   (index "));   Serial.print(sel);
  Serial.print(F(", PCA9685 channel ")); Serial.print(servoSetup[sel][1]);
  Serial.println(F(")"));
  Serial.print(F("  you should see: ")); Serial.println(servoJoint[sel]);
  Serial.print(F("  position "));  Serial.print((int)servoPos[sel]);
  Serial.print(F("   home "));     Serial.print((int)servoHome[sel]);
  Serial.print(F("   range "));    Serial.print((int)min(servoLimit[sel][0], servoLimit[sel][1]));
  Serial.print(F(" .. "));         Serial.println((int)max(servoLimit[sel][0], servoLimit[sel][1]));
}

// Turn servo power on or off. OE (output enable) on the PCA9685 is ACTIVE
// LOW: pulling the pin LOW enables the outputs, HIGH disables them. Disabled
// means every servo goes limp at once, regardless of what we last commanded.
void setOutput(byte on) {
  outputOn = on;
  digitalWrite(OE_PIN, on ? LOW : HIGH);
  digitalWrite(LED_PIN, on ? HIGH : LOW);   // orange LED lit = servos live
  Serial.println(on ? F("\n*** SERVO POWER ON - it can move now ***")
                    : F("\n*** servo power off - everything limp ***"));
}


/* =====================================================================
   SECTION 5: SETUP - runs once at boot
   ===================================================================== */

void setup() {
  // Disable servo output BEFORE anything else, so a reset mid-test cannot
  // make the robot lurch. OE is active LOW, so HIGH = outputs off. We drive
  // it high immediately after pinMode, because until pinMode runs the pin is
  // an input and its state is whatever the driver board's pullup decides.
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 5000);   // wait up to 5s for the monitor, then
                                        // carry on regardless so this still
                                        // runs untethered
  Serial.println(F("\n=== Nova SM3 :: single-servo test (Teensy 4.0) ==="));
  Serial.println(F("*** ROBOT ON A STAND, LEGS FREE, before you press START ***"));

  // --- find the servo driver on the i2c bus ---
  Wire.begin();
  Serial.println(F("\nScanning i2c for the PCA9685..."));
  byte found = 0;
  for (byte a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  device at 0x")); Serial.print(a, HEX);
      if (a == 0x40) { Serial.print(F("   <-- the servo driver")); found = 1; }
      if (a == 0x01) Serial.print(F("   <-- the Nano slave board"));
      Serial.println();
    }
  }
  if (!found) {
    Serial.println(F("  NOT FOUND. Check 5V, GND, SDA and SCL, and that the"));
    Serial.println(F("  board has its i2c pullup resistors. Nothing will move."));
  }

  // --- start the driver ---
  pwm1.begin();
  pwm1.setOscillatorFrequency(OSCIL_FREQ);
  pwm1.setPWMFreq(SERVO_FREQ);
  delay(10);

  // Load every servo's home position into the driver. Output is still
  // disabled, so nothing moves yet - this just means that the instant you
  // press START, the servos go to a known sane pose instead of somewhere
  // random left over from the last run.
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    servoPos[i] = servoHome[i];
    moveServo(i, servoHome[i]);
  }
  Serial.println(F("\nAll servos staged at home (still powered off)."));

  // --- connect the PS2 controller ---
  Serial.print(F("\nLooking for the PS2 controller"));
  for (byte attempt = 0; attempt < 10; attempt++) {
    // NOTE: this is the Teensy PS2X fork, where config_gamepad returns
    // 0 on SUCCESS. The stock library returns non-zero. Do not "fix" this.
    if (ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false) == 0) {
      ps2Ready = 1;
      break;
    }
    Serial.print(F("."));
    delay(500);
  }
  if (ps2Ready) {
    Serial.println(F("  found."));
  } else {
    Serial.println(F("  NOT FOUND."));
    Serial.println(F("  Is the receiver LED solid (paired) rather than blinking?"));
    Serial.println(F("  Check DAT 7 / CMD 8 / SEL 9 / CLK 10, and that it is on"));
    Serial.println(F("  3.3V - the Teensy 4.0 is NOT 5V tolerant."));
    Serial.println(F("  You can still drive this test from the serial keys below."));
  }

  showControls();
  announceServo();
}

void showControls() {
  Serial.println(F("\n--- controls ---"));
  Serial.println(F("  PS2  START        enable servo power"));
  Serial.println(F("  PS2  SELECT       disable servo power"));
  Serial.println(F("  PS2  D-pad UP     HOLD to rotate one way"));
  Serial.println(F("  PS2  D-pad DOWN   HOLD to rotate the other way"));
  Serial.println(F("  PS2  D-pad RIGHT  next servo"));
  Serial.println(F("  PS2  D-pad LEFT   previous servo"));
  Serial.println(F("  PS2  CROSS (X)    send this servo home"));
  Serial.println(F("  backup keys (if the controller drops):"));
  Serial.println(F("    e / d  power on / off      n / p  next / prev servo"));
  Serial.println(F("    + / -  nudge one step      h      home this servo"));
  Serial.println(F("    ?      show this again"));
  Serial.println(F("----------------\n"));
}


/* =====================================================================
   SECTION 6: LOOP - runs over and over, forever

   Nothing here blocks. We check the clock, and only read the controller
   when PS2_INTERVAL ms have gone by. That keeps the loop responsive.
   ===================================================================== */

void loop() {
  handleSerialKeys();   // backup keyboard control, always available

  if (!ps2Ready) return;
  if (millis() - lastPS2Read < PS2_INTERVAL) return;
  lastPS2Read = millis();

  ps2x.read_gamepad(false, false);

  // --- power on / off ---
  // ButtonPressed() fires ONCE on the press. That is what we want for
  // things that toggle - otherwise holding START would fire 20x a second.
  if (ps2x.ButtonPressed(PSB_START))  setOutput(1);
  if (ps2x.ButtonPressed(PSB_SELECT)) setOutput(0);

  // --- change which servo we are driving ---
  if (ps2x.ButtonPressed(PSB_PAD_RIGHT)) {
    sel = (sel + 1) % TOTAL_SERVOS;
    announceServo();
  }
  if (ps2x.ButtonPressed(PSB_PAD_LEFT)) {
    sel = (sel + TOTAL_SERVOS - 1) % TOTAL_SERVOS;
    announceServo();
  }

  // --- send it home ---
  if (ps2x.ButtonPressed(PSB_CROSS)) {
    moveServo(sel, servoHome[sel]);
    Serial.print(F("  home -> ")); Serial.println((int)servoPos[sel]);
  }

  // --- the actual jog, one way or the other ---
  // Button() (not ButtonPressed) is TRUE the whole time the button is held
  // down. So every poll that UP is held, we add JOG_STEP ticks. Let go and
  // it stops. That is what makes it feel like holding a motor button.
  if (outputOn) {
    if (ps2x.Button(PSB_PAD_UP))   jog(+JOG_STEP);
    if (ps2x.Button(PSB_PAD_DOWN)) jog(-JOG_STEP);
  } else if (ps2x.Button(PSB_PAD_UP) || ps2x.Button(PSB_PAD_DOWN)) {
    // Held a jog button with the power off - say so, but only occasionally
    // so we do not flood the monitor 20 times a second.
    static unsigned long lastNag = 0;
    if (millis() - lastNag > 1500) {
      lastNag = millis();
      Serial.println(F("  (servo power is OFF - press START first)"));
    }
  }
}

// Move the selected servo by `delta` ticks and report it.
void jog(int delta) {
  float before = servoPos[sel];
  moveServo(sel, servoPos[sel] + delta);

  // If the position did not change, we are sitting on a travel limit.
  if (servoPos[sel] == before) {
    static unsigned long lastLimitMsg = 0;
    if (millis() - lastLimitMsg > 1000) {
      lastLimitMsg = millis();
      Serial.print(F("  ")); Serial.print(servoName[sel]);
      Serial.print(F(" at its limit ("));
      Serial.print((int)before); Serial.println(F(") - will not go further"));
    }
    return;
  }

  // Print every few ticks rather than every one, so the log stays readable.
  static int sinceLastPrint = 0;
  sinceLastPrint += abs(delta);
  if (sinceLastPrint >= 10) {
    sinceLastPrint = 0;
    Serial.print(F("  ")); Serial.print(servoName[sel]);
    Serial.print(F(" (ch ")); Serial.print(servoSetup[sel][1]);
    Serial.print(F(")  ")); Serial.println((int)servoPos[sel]);
  }
}


/* =====================================================================
   SECTION 7: BACKUP KEYBOARD CONTROL

   The PS2 link is wireless and can drop. Being unable to cut servo power
   from the keyboard when that happens is a safety problem, so these stay.
   ===================================================================== */

void handleSerialKeys() {
  if (!Serial.available()) return;
  char c = Serial.read();

  switch (c) {
    case 'e': setOutput(1); break;
    case 'd': setOutput(0); break;
    case 'n': sel = (sel + 1) % TOTAL_SERVOS;                 announceServo(); break;
    case 'p': sel = (sel + TOTAL_SERVOS - 1) % TOTAL_SERVOS;  announceServo(); break;
    case 'h': moveServo(sel, servoHome[sel]);
              Serial.print(F("  home -> ")); Serial.println((int)servoPos[sel]); break;
    case '+': jog(+JOG_STEP); break;
    case '-': jog(-JOG_STEP); break;
    case '?': showControls(); announceServo(); break;
    default:  break;    // ignore newlines and anything else
  }
}
