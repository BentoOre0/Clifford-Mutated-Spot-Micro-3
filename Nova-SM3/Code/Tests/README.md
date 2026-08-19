# Nova SM3 — peripheral bench tests

Standalone, single-purpose sketches for isolating one piece of hardware at a
time. Each one is independent: flash it, confirm that subsystem works, move on.
None of them include `NovaServos.h`, `AsyncServo.h`, or any other main-sketch
file, so a pass here proves *wiring and hardware*, not the main firmware.

Pin assignments are copied from `Nova-SM3_teensy-v5.0` and `Nova-SM3_nano-v5.1`.
If you change a pin in the main sketches, change it here too or these stop
being a valid reference.

## Which board

| Sketch | Board | Peripheral |
|---|---|---|
| `Test_MPU6050` | Teensy 4.0 | MPU6050 IMU on `Wire1` (SDA2 17 / SCL2 16, addr 0x68) |
| `Test_PWM_Servos` | Teensy 4.0 | PCA9685 @ 0x40 + 12 servos, OE pin 3 |
| `Test_PIR` | Teensy 4.0 | 3× PIR (front 4, left 5, right 6) |
| `Test_PS2` | Teensy 4.0 | PS2 receiver (DAT 7, CMD 8, SEL 9, CLK 10) |
| `Test_MP3` | Teensy 4.0 | DFPlayer Mini (RX 0, TX 1, vol pot 20) |
| `Test_Ultrasonic` | Arduino Nano | 2× HC-SR04 (L 7/6, R 5/4) |
| `Test_OLED` | Arduino Nano | SSD1331 96×64 SPI (13/11/10/9/8) |
| `Test_NeoPixel` | Arduino Nano | 4× WS2812 eyes (pin 2) + brightness pot A3 |

Set the correct board in the IDE before uploading — the Teensy sketches will
not compile for a Nano and vice versa.

## Libraries

Point the IDE at `../Arduino Libraries/`, or:

```
arduino-cli compile --fqbn <fqbn> --libraries "../Arduino Libraries" Test_MPU6050
```

Everything is vendored **except** `DFRobotDFPlayerMini` (needed by `Test_MP3`),
which you must install via Library Manager. The vendored `DFRobot_DF1201S.zip`
is for the DFPlayer Mini **Pro**, a different module with a different API.

## Suggested order

Work outward from the things everything else depends on:

1. **`Test_MPU6050`** — also scans `Wire1`, so it doubles as an I²C bus check.
2. **`Test_PWM_Servos`** — scans the default `Wire` bus (should find `0x40`,
   and `0x01` if the Nano is powered). Confirms the master↔slave link exists
   before you trust either side.
3. **`Test_PS2`**, **`Test_PIR`**, **`Test_MP3`** — remaining Teensy peripherals.
4. **`Test_OLED`**, **`Test_NeoPixel`**, **`Test_Ultrasonic`** — Nano side.

## ⚠️ Before running `Test_PWM_Servos`

- **Put the robot on a stand with the legs hanging free.**
- Output is disabled at boot (`OE` held HIGH). Nothing moves until you type `e`.
- All moves are clamped to `servoLimit[]` from `NovaServos.h`.
- Note those limit pairs are `{min,max}` in *leg* direction, not numeric order
  (e.g. `LFF` is `{537,207}`), so the sketch clamps against the numeric lo/hi.
- The real thing this test catches: `servoSetup[]` maps servos to PCA9685
  channels 0,1,2 / 4,5,6 / 8,9,10 / 12,13,14 — channels 3, 7, 11, 15 are
  deliberately skipped. A mis-wire produces **no error**, just the wrong joint
  moving. Jog each servo and confirm the expected joint responds.

## Serial

All sketches use **115200 baud**. Teensy sketches wait up to 5s for a serial
connection then continue regardless, so they won't hang if run untethered —
unlike the main sketch, which will not boot without serial if any `debug` flag
is set.
