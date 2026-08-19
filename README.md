# Modded Nova SM3

Firmware for **Nova SM3**, a Spot-Mini Micro clone quadruped robot — a modded
fork of [Chris Locke's Nova SM3](https://novaspotmicro.com)
([upstream repo](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3)).

**This repository is code only.** The printable STLs, wiring diagrams, MP3
assets and parts list are large and unchanged from upstream — get those from
the upstream repo linked above.

## Start here

**[`Nova-SM3/Code/README.md`](Nova-SM3/Code/README.md)** — the real
documentation. It covers the v6.0 sketch layout, the Teensy/Nano master-slave
split, the non-blocking loop model, and the hardware gotchas worth knowing
before you flash anything (active-low OE, 3.3V-only inputs, debug flags that
block an untethered boot, A1 shared between current and battery sensing).

## Sketches

Each directory is a **complete, independent Arduino sketch** — they are
snapshots, not diffs of each other.

| Sketch | Board | Role |
| --- | --- | --- |
| `Nova-SM3_teensy-v6.0/` | Teensy 4.0 | **Current master** — servos, IMU, PS2, sensors, serial commands |
| `Nova-SM3_nano-v6.0/` | Arduino Nano | **Current slave** — OLED, NeoPixels, ultrasonics, EEPROM |
| `Nova-SM3_teensy-v5.1/` + `Nova-SM3_nano-v5.1/` | Teensy 4.0 / Nano | The pair v6.0 came from, kept unmodified as a behavioural reference |
| `Nova-SM3_teensy-v5.0/` | Teensy 4.0 | Older master, predates DFPlayer Mini MP3 support |
| `Nova-SM3_teensy-v4.2/` + `Nova-SM3_nano-v4.2/` | Mega / Nano | Previous generation, predates the Teensy port |
| `Nova-SM3-calibrate/` | Teensy 4.0 | Bench sketch for calibrating servo home/min/max into `NovaServos.h` |
| `Tests/` | either | Standalone bring-up sketches, one subsystem at a time |

`Arduino Libraries/` holds vendored third-party dependencies.

## Changes in this fork

- **v6.0 master/slave pair**, derived from v5.1. The monolithic `.ino` is split
  into topical tabs, with pins and feature flags pulled out into `NovaConfig.h`
  and the I2C command bytes into a shared `NovaSlaveProtocol.h`, so master and
  slave cannot disagree about what a byte means.
- **`Code/Tests/`** — standalone sketches to bring up each subsystem (servos,
  IMU, OLED, NeoPixels, ultrasonic, PIR, PS2, MP3) individually, so a hardware
  fault can be isolated without flashing the full firmware.
- Superseded pre-v4.2 sketch directories removed.
- Modified STL files (not tracked here — see above).

## Building

No build system; these are plain Arduino sketches.

```sh
arduino-cli compile --fqbn teensy:avr:teensy40 --libraries "Nova-SM3/Code/Arduino Libraries" Nova-SM3/Code/Nova-SM3_teensy-v6.0
arduino-cli compile --fqbn arduino:avr:nano     --libraries "Nova-SM3/Code/Arduino Libraries" Nova-SM3/Code/Nova-SM3_nano-v6.0
```

`DFRobotDFPlayerMini` is not vendored and must be installed separately. In the
Arduino IDE, point the sketchbook library path at `Nova-SM3/Code/Arduino Libraries`
and install Teensyduino for the Teensy boards.

Validation is manual: flash to hardware and exercise it over the serial monitor
command interface or a PS2 controller.

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2021 Christopher M. Locke.
Nova SM3 is his design and his code: the mechanical design, the gait
development, the servo motion engine and the master/slave architecture.
