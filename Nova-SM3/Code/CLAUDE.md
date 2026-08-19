# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Nova SM3 is the firmware for a Spot-Mini Micro quadruped robot clone (hobby/open-source project by Chris Locke, https://novaspotmicro.com). This directory contains the Arduino sketches; STL files, wiring diagrams, and the parts list live one level up in `../` (see `../README.md`). The code is explicitly *not* written with public-consumption cleanliness in mind — expect large monolithic `.ino` files, global mutable state, and TODO/DEVWORK comments left in place.

## Repo layout

Each top-level directory is a **complete, independent Arduino sketch** for a specific board/version combo — they are snapshots, not diffs of each other:

- `Nova-SM3_teensy-v6.0/` — **current main sketch**, runs on a Teensy 4.0 as the **master** controller (servos, IMU, PS2 remote, sensors, serial commands). Unlike the older versions it is split across several `.ino` tabs plus `NovaConfig.h` (flags/pins), `NovaServos.h` (calibration), `NovaSlaveProtocol.h` (i2c command names) and `AsyncServo.h`.
- `Nova-SM3_nano-v6.0/` — **current slave sketch**, runs on an Arduino Nano, drives the OLED display, RGB NeoPixels, ultrasonic sensors, and EEPROM-persisted settings.
- `Nova-SM3_teensy-v5.1/` / `Nova-SM3_nano-v5.1/` — the pair v6.0 was derived from, kept unmodified as a behavioural reference.
- `Nova-SM3_teensy-v5.0/` — older master, predates the DFPlayer Mini MP3 support.
- `Nova-SM3_teensy-v4.2/` / `Nova-SM3_nano-v4.2/` — previous generation master/slave pair (predates the Teensy 4.0 port, when a Mega was the master).

**Read `README.md` first.** It documents the v6.0 structure, the master/slave split, the non-blocking loop model, and the hardware gotchas (active-low OE pin, 3.3V-only inputs, debug flags blocking untethered boot, A1 shared between current and battery sensing).
- `Nova-SM3-calibrate/` — standalone throwaway sketch (not master/slave) used to physically calibrate servo home/min/max positions on the bench before copying values into `NovaServos.h`.
- `Arduino Libraries/` — vendored copies of third-party libraries (Adafruit GFX/PWM/NeoPixel/SSD1331, PS2X_lib, NewPing, T4_PowerButton, EEPROM, SPI, Wire) that these sketches depend on; point the Arduino IDE's sketchbook/library path here, or `arduino-cli --libraries "Arduino Libraries"`.

Because each version directory is a full independent copy, a bug fix or feature typically needs to be manually ported across the relevant version directories (at minimum the current `teensy-v5.0` + `nano-v5.1` pair) — there is no shared/common code module between them.

## Build / compile

There is no build system, test suite, or linter — this is plain Arduino IDE sketch code. To compile:

- **Arduino IDE**: open the `.ino` file for the target sketch directory (e.g. `Nova-SM3_teensy-v6.0/Nova-SM3_teensy-v6.0.ino`), select the correct board (Teensy 4.0 vs Arduino Nano), and Verify/Upload. Install the Teensyduino add-on for Teensy boards.
- **arduino-cli**: `arduino-cli compile --fqbn teensy:avr:teensy40 --libraries "Arduino Libraries" Nova-SM3_teensy-v6.0` and `arduino-cli compile --fqbn arduino:avr:nano --libraries "Arduino Libraries" Nova-SM3_nano-v6.0`. Both compile clean. `DFRobotDFPlayerMini` is NOT vendored and must be installed separately.
- Validation is manual/physical: flash to hardware and exercise it via the serial monitor command interface (see below) or a PS2 controller — there's no way to test this code without a real board attached.

## Architecture (teensy-v6.0 master / nano-v6.0 slave)

The description below was written against the v5.x pair and still describes how the
system works. In v6.0 the same code is split across topical `.ino` tabs — see
`README.md` for the file-by-file map, and `NovaSlaveProtocol.h` for the master/slave
command bytes, which are now named constants rather than bare strings.

**Two-board I2C setup.** The Teensy is I2C master and does all the "thinking": servo control, IMU fusion, PS2 input, serial commands, PIR/ultrasonic sensor logic, battery/amperage monitoring. The Nano is I2C slave (`SLAVE_ID` = 1) and only handles peripheral I/O it owns directly (OLED, RGB LEDs, its own ultrasonic sensors, EEPROM). They talk via a tiny single-byte-command protocol:
  - Master: `command_slave(char*)` in the `.ino` (around line 5652) writes command byte(s) via `Wire1.beginTransmission`/`requestFrom`.
  - Slave: `receiveEvent()` stores the incoming byte into `req`; `requestCallback()` (`Wire.onRequest`) switches on `req` to act and returns a response byte. There's a `serial_oled` mode flag that switches the meaning of incoming bytes between "system command" and "OLED command".

**Main loop is a non-blocking state machine**, not blocking sequential code:
  - `loop()` first calls `update_servos()` every iteration (this drives all 12 `AsyncServo` objects), then checks a long `if/else if` chain of `move_*` boolean flags (`move_forward`, `move_trot`, `move_roll`, `move_march`, `move_sequence`, ...) to decide which single movement function to advance this tick, then polls each subsystem (PS2, serial, PIR, MPU, ultrasonic, amperage, battery) against its own `lastXUpdate`/`XInterval` millis() timer.
  - Movement functions (`step_forward`, `step_trot`, `roll`, `pitch`, `wake`, `wman`, `run_sequence`, etc.) don't loop internally — they set `targetPos[]`/`servoSpeed[]` for the relevant servos and return; actual motion happens incrementally inside `AsyncServo::Update()` on subsequent loop iterations.

**`AsyncServo` (`AsyncServo.h`)** is the servo-motion engine. Despite being a "class", it reads/writes external global arrays (`servoPos`, `targetPos`, `servoSpeed`, `activeServo`, `servoRamp`, `servoSweep`, `servoDelay`, ...) rather than encapsulating its own state — this is called out explicitly in the file's header comment as a known wart. `Update()` is called once per servo per `loop()` and: advances position by one PWM tick toward `targetPos` when the per-servo speed interval has elapsed, applies acceleration/deceleration ramping (`servoRamp`, controlled by `use_ramp`), handles continuous back-and-forth "sweep" mode (`activeSweep`), and triggers `amperage_check()` when a servo reaches its target (to catch stalled/jammed motors).

**`NovaServos.h`** is the per-robot hardware calibration data: servo IDs (`RFC`/`RFF`/`RFT`/... = right-front coxa/femur/tibia, etc.), which PWM board/pin each servo is wired to (`servoSetup`), and — critically — `servoHome[]` and `servoLimit[]`, the physically-calibrated home position and min/max travel for each servo. These values are specific to one assembled robot and are normally only regenerated via the `Nova-SM3-calibrate` sketch; don't "fix" them without understanding they encode physical calibration, not logic. `SERVO_FREQ`/`OSCIL_FREQ` in the main `.ino` carry an explicit "CAUTION: do not change once calibrated" comment for the same reason.

**`MPU6050_conf.h`** is a register-level MPU6050 IMU driver (based on kriswiner's library) providing `initMPU6050()`, `calibrateMPU6050()`, `MPU6050SelfTest()`, and raw accel/gyro read functions over the Teensy's second I2C bus (`Wire1`).

**Debug/feature flags.** Near the top of the main `.ino`, a block of `const byte debug`/`debug1`...`debug7`/`plotter` flags gate different categories of `Serial.print` diagnostics (general, PS2/PIR, servo steps, ramping/sequencing, amperage, MPU, serial/oled comms, ultrasonic), and a separate block of `byte *_active` flags (`pwm_active`, `ps2_active`, `mpu_active`, `rgb_active`, `oled_active`, ...) toggles whole subsystems on/off. On Teensy specifically, any debug flag left on requires an attached serial connection to boot — there's a code comment warning to set all debug flags to 0 before running untethered.

**Serial command interface.** When `serial_active`, typed commands terminated by `,` (or newline) are parsed by `serial_check()`/`serial_command()` (`.ino`, ~line 5041) — this is the primary way to drive/debug the robot from a PC without a PS2 controller (e.g. `stop`, `v`/`vars`, speed presets `1`-`9`, `t`/trot, `m`/march, etc.).
