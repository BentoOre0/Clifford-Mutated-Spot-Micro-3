# Electronics

Power, wiring, assembly and bring-up for the modified Nova SM3.

← [back to the main README](../README.md)

---

## What the robot has to power

Twelve servos, two microcontrollers, an IMU, three PIR sensors, four ultrasonic
sensors, an OLED display, four addressable RGB LEDs, an MP3 player and a speaker — from
one LiPo pack.

Servos dominate. Twelve of them stalling simultaneously is a very different load from
twelve of them idling, and the transient when a gait starts is the worst case. This is
the constraint the rest of the electrical design has to survive.

---

## The bring-up strategy

The decision that mattered most here was **not** to assemble everything, flash the full
firmware, and start guessing.

The inherited firmware is large, it drives many peripherals, and a fault could be in my
soldering, my wiring, my power rail, or somebody else's code. Debugging all of those
simultaneously is how you lose a week.

So I wrote [`Nova-SM3/Code/Tests/`](../Nova-SM3/Code/Tests/) — eight standalone sketches,
one per peripheral, each of which deliberately includes **none** of the main firmware:

> *"None of them include `NovaServos.h`, `AsyncServo.h`, or any other main-sketch file,
> so a pass here proves wiring and hardware, not the main firmware."*

| Sketch | Board | Under test | Interface |
|---|---|---|---|
| `Test_PWM_Servos` | Teensy 4.0 | PCA9685 + 12 servos | I²C @ 0x40, OE pin 3 |
| `Test_PS2` | Teensy 4.0 | PS2 receiver | DAT 7, CMD 8, SEL 9, CLK 10 |
| `Test_MPU6050` | Teensy 4.0 | IMU | `Wire1` — SDA2 17 / SCL2 16, addr 0x68 |
| `Test_PIR` | Teensy 4.0 | 3× PIR | front 4, left 5, right 6 |
| `Test_MP3` | Teensy 4.0 | DFPlayer Mini | RX 0, TX 1, volume pot 20 |
| `Test_Ultrasonic` | Arduino Nano | 2× HC-SR04 | L 7/6, R 5/4 |
| `Test_OLED` | Arduino Nano | SSD1331 96×64 | SPI 13/11/10/9/8 |
| `Test_NeoPixel` | Arduino Nano | 4× WS2812 | pin 2, brightness pot A3 |

Each carries an explicit **pass criterion**, so "it did something" is not mistaken for
"it works". This is also why the pin map above is trustworthy — it is the map the
hardware was actually verified against.

---

## Hazards found and written down

These are in the source as warnings because they were live problems during bring-up, not
hypotheticals.

### The Teensy 4.0 is not 5 V tolerant

From [`Test_PS2.ino`](../Nova-SM3/Code/Tests/Test_PS2/Test_PS2.ino):

> *"The PS2 receiver is a 3.3V part. The Teensy 4.0 is ALSO 3.3V and is NOT 5V tolerant —
> do not feed the receiver 5V and then wire DAT back to the Teensy."*

The failure mode is nasty because it is not obvious: the receiver *works* on 5 V. It
just quietly destroys the microcontroller through the data line.

### `OE_PIN` is active LOW, and the boot order matters

Driving OE **low** *enables* the servos. It is held high through boot until the PS2 link
is up, because the PWM driver interferes with the receiver — without that sequencing the
robot lurches on power-up. Servo output arms roughly a second after boot.

### Current sense and battery sense share one pin

`AMP_PIN` and `BATT_MONITOR` are **both A1**. This robot physically cannot measure servo
current and battery voltage at the same time; enabling both `amp_active` and
`batt_active` without rewiring gives you nonsense from one of them. This is inherited
from the original design, and it is a genuine architectural limitation rather than a
bug.

### Debug flags block an untethered boot

On a Teensy, printing to a serial port with nothing attached stalls the board. Any debug
flag left set in `NovaConfig.h` means the robot appears completely dead when you unplug
it from the laptop. Set them all to 0 before running on battery.

---

## Power distribution

![Clifford during electronics integration](../hardware/media/assembled-electronics-integration.jpg)

Visible in the build: the servo driver board, a buck converter (the green board with the
toroidal inductor), an XT60 connector for the LiPo, the twelve servo looms, the speaker,
and hand-written labels on the wiring.

I designed the power distribution around the converters I could actually buy rather than
the ones specified, which meant working out the rail structure myself rather than
following the original wiring document.

> **Evidence note — cascading buck converters.** In my own project notes I recorded
> debugging power distribution problems caused by **cascading buck converters** — one
> converter fed from the output of another, which is an arrangement that tends to
> misbehave under the kind of load transient twelve servos produce, and which compounds
> conversion losses. **This repository contains no measurements, no scope captures and no
> written fault log to substantiate that**, so I am recording it as what it is: a problem
> I remember diagnosing, not a documented result. The same applies to the continuity and
> resistance tracing I did across the boards — I have no recorded measurements from it.

---

## Wiring diagram

**Not yet in this repository.**

I produced a modified wiring diagram reflecting the components I actually sourced, which
differs from Chris Locke's original because the converters and several peripherals are
not the specified parts. It was not available when this documentation was assembled.

When it lands it belongs in [`../hardware/electronics/`](../hardware/electronics/)
alongside a short note on what changed relative to the original wiring architecture. It
should be committed as the original artifact — PDF or image, as drawn — not redrawn or
regenerated.

Upstream's original wiring and schematic documents are not reproduced here; they are
available from
[the upstream project](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3).

---

## Honest gaps

- **No schematic of my modifications.** The pin map is verified; the power tree is not
  drawn.
- **No measurements.** No current draw figures, no rail voltages under load, no
  continuity/resistance log.
- **No PCB photographs** beyond the integration shot above.
- **No component list** for the substituted parts — the same omission as the servos in
  [the mechanical write-up](mechanical-redesign.md#what-is-not-documented-here), and the
  thing I would most want to have recorded.
