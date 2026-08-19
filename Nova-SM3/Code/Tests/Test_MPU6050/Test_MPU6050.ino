/*
 *   Nova SM3 - Bench Test: MPU6050 IMU
 *   TARGET BOARD: Teensy 4.0   (master)
 *
 *   Verifies the IMU on the Teensy's SECOND i2c bus (Wire1).
 *   Pins match Nova-SM3_teensy-v5.0: SDA2 = 17, SCL2 = 16, addr 0x68.
 *
 *   This is a raw-register test - it deliberately does NOT use MPU6050_conf.h,
 *   so a pass here proves the wiring/bus, not the driver.
 *
 *   PASS CRITERIA:
 *     1. "WHO_AM_I = 0x68" (0x00 or 0xFF means wiring/power problem)
 *     2. Accel Z reads roughly +1.0 g with the robot sat level
 *     3. Tilting the board changes X/Y smoothly, no wild jumps
 */

#include <Wire.h>

#define SDA2_PIN 17
#define SCL2_PIN 16
#define LED_PIN  13

const int MPU = 0x68;

// MPU6050 registers
#define REG_SELF_TEST_X 0x0D
#define REG_SMPLRT_DIV  0x19
#define REG_CONFIG      0x1A
#define REG_GYRO_CONF   0x1B
#define REG_ACCEL_CONF  0x1C
#define REG_ACCEL_XOUT  0x3B
#define REG_PWR_MGMT_1  0x6B
#define REG_WHO_AM_I    0x75

// full-scale ranges selected below: accel +/-2g, gyro +/-250 dps
const float ACCEL_SCALE = 16384.0;
const float GYRO_SCALE  = 131.0;

unsigned long lastRead = 0;
const unsigned int readInterval = 200;

void writeByte(uint8_t addr, uint8_t reg, uint8_t data) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  Wire1.write(data);
  Wire1.endTransmission();
}

uint8_t readByte(uint8_t addr, uint8_t reg) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  Wire1.endTransmission(false);
  Wire1.requestFrom(addr, (uint8_t)1);
  return Wire1.available() ? Wire1.read() : 0xFF;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 5000);   // don't hang forever if run untethered

  Serial.println(F("\n=== Nova SM3 :: MPU6050 test (Teensy 4.0, Wire1) ==="));
  Serial.print(F("SDA2 = ")); Serial.print(SDA2_PIN);
  Serial.print(F("   SCL2 = ")); Serial.println(SCL2_PIN);

  Wire1.setSDA(SDA2_PIN);
  Wire1.setSCL(SCL2_PIN);
  Wire1.begin();
  Wire1.setClock(400000);
  delay(100);

  // --- bus scan, so a wrong-address module still shows up ---
  Serial.println(F("\nScanning Wire1..."));
  byte found = 0;
  for (byte a = 1; a < 127; a++) {
    Wire1.beginTransmission(a);
    if (Wire1.endTransmission() == 0) {
      Serial.print(F("  device at 0x"));
      Serial.println(a, HEX);
      found++;
    }
  }
  if (!found) {
    Serial.println(F("  NONE FOUND -> check 3.3V, GND, SDA/SCL swap, pullups"));
  }

  // --- identity ---
  uint8_t who = readByte(MPU, REG_WHO_AM_I);
  Serial.print(F("\nWHO_AM_I = 0x"));
  Serial.print(who, HEX);
  if (who == 0x68 || who == 0x72) {
    Serial.println(F("   <-- OK"));
  } else {
    Serial.println(F("   <-- FAIL (expected 0x68)"));
    Serial.println(F("Halting. Fix wiring before continuing."));
    while (1) {                      // slow blink = hard fail
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
  }

  // --- wake + configure ---
  writeByte(MPU, REG_PWR_MGMT_1, 0x00);   // wake from sleep
  delay(100);
  writeByte(MPU, REG_PWR_MGMT_1, 0x01);   // clock ref = gyro X (more stable)
  writeByte(MPU, REG_CONFIG,     0x03);   // DLPF 44Hz
  writeByte(MPU, REG_SMPLRT_DIV, 0x04);   // 200Hz sample rate
  writeByte(MPU, REG_GYRO_CONF,  0x00);   // +/-250 dps
  writeByte(MPU, REG_ACCEL_CONF, 0x00);   // +/-2 g
  delay(100);

  Serial.println(F("\nStreaming. Sit the robot LEVEL: expect az ~ +1.00 g\n"));
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  if (millis() - lastRead < readInterval) return;
  lastRead = millis();

  Wire1.beginTransmission(MPU);
  Wire1.write(REG_ACCEL_XOUT);
  Wire1.endTransmission(false);
  Wire1.requestFrom((uint8_t)MPU, (uint8_t)14);

  if (Wire1.available() < 14) {
    Serial.println(F("read failed - bus dropped out"));
    return;
  }

  int16_t ax = Wire1.read() << 8 | Wire1.read();
  int16_t ay = Wire1.read() << 8 | Wire1.read();
  int16_t az = Wire1.read() << 8 | Wire1.read();
  int16_t t  = Wire1.read() << 8 | Wire1.read();
  int16_t gx = Wire1.read() << 8 | Wire1.read();
  int16_t gy = Wire1.read() << 8 | Wire1.read();
  int16_t gz = Wire1.read() << 8 | Wire1.read();

  float axg = ax / ACCEL_SCALE;
  float ayg = ay / ACCEL_SCALE;
  float azg = az / ACCEL_SCALE;

  // derived tilt, so you can sanity-check against the real robot pose
  float roll  = atan2(ayg, azg) * 57.2958;
  float pitch = atan2(-axg, sqrt(ayg * ayg + azg * azg)) * 57.2958;

  Serial.print(F("ax:")); Serial.print(axg, 2);
  Serial.print(F("  ay:")); Serial.print(ayg, 2);
  Serial.print(F("  az:")); Serial.print(azg, 2);
  Serial.print(F("  |  gx:")); Serial.print(gx / GYRO_SCALE, 1);
  Serial.print(F("  gy:")); Serial.print(gy / GYRO_SCALE, 1);
  Serial.print(F("  gz:")); Serial.print(gz / GYRO_SCALE, 1);
  Serial.print(F("  |  roll:")); Serial.print(roll, 1);
  Serial.print(F("  pitch:")); Serial.print(pitch, 1);
  Serial.print(F("  |  temp:")); Serial.print((t / 340.0) + 36.53, 1);
  Serial.println(F("C"));
}
