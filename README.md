# Modded Nova SM3 — "Clifford"

A twelve-servo quadruped robot, built from [Chris Locke's Nova SM3](https://novaspotmicro.com)
design and then substantially re-engineered because I could not buy the parts it
specifies.

![Clifford mid-integration](hardware/media/assembled-electronics-integration.jpg)

*Clifford on the bench during electronics integration. Grey and red parts are my
prints; the leg segments visible top and bottom are my redesigned geometry. Note the
buck converter (green board, toroidal inductor), the servo driver board, the labelled
wiring, and the calipers — the redesign was measurement-driven.*

---

## My role, in one paragraph

**This is not my design.** The Nova SM3 — its architecture, its gaits, its servo motion
engine, its master/slave split — is Chris Locke's work, and the firmware in this repo
is his code. What is mine is everything that had to change when the specified hardware
turned out to be unavailable where I live: **I redesigned the hip, femur and tibia leg
parts around servos with different mounting geometry, printed and assembled the whole
robot, wired and debugged the electronics and power distribution, brought each
subsystem up individually with test sketches I wrote, and reorganised 6,390 lines of
inherited firmware into something maintainable.** The robot stands, the electronics
work, and the controller talks to the servos. It does not yet walk.

---

## The constraint that drove everything

I built this in the Philippines. A significant share of the components in the original
bill of materials were not practically sourceable locally, and the servos were the ones
that mattered.

Servos are not interchangeable. Swapping to a different servo changes:

- the **body dimensions**, so every pocket and bracket that captures the case is wrong;
- the **mounting hole pattern**, so every screw boss is wrong;
- but *not* necessarily the **output horn spline**, which is the one thing that must not
  move, because it defines each joint's axis and therefore the robot's kinematics.

So the redesign had a clear rule: **preserve the horn interface exactly, rebuild
everything around it.** That rule is visible in every comparison below.

---

## What I changed, and what I didn't

| Area | Chris Locke's | Mine |
|---|---|---|
| Robot architecture, gaits, `AsyncServo` motion engine, master/slave I²C protocol | ✅ all of it | — |
| Original STL geometry | ✅ | Used as the base body, then modified |
| **Hip (coxa), femur, tibia geometry** | Original geometry | ✅ **Redesigned around replacement servos** |
| Printing and assembly of the whole robot | — | ✅ |
| Wiring, power distribution, PCB assembly and bring-up | — | ✅ |
| Per-subsystem bench test sketches (`Code/Tests/`) | — | ✅ |
| Firmware v4.2 → v5.1 | ✅ | — |
| **Firmware v6.0** (restructure, documentation, 2 bug fixes) | Behaviour is his | ✅ **Reorganisation is mine** |
| Servo calibration values | ✅ | Still his — see [Status](#status) |

---

## Mechanical redesign — the evidence

Every figure below is rendered directly from the actual STL files in
[`hardware/cad/`](hardware/cad/), original and modified, at matched camera angles.
Dimensions and volumes are measured from the mesh geometry, not estimated.

### Hip / coxa bracket

![Original vs modified coxa bracket](hardware/cad/comparisons/coax-comparison.png)

The servo-horn boss, its arc rib and the retaining tabs carry straight over — and the
envelope along the joint axis matches to within **0.14 mm**. Everything that holds the
servo body is new: the original's full-height back plate and overhanging two-hole ear
become a stepped C-bracket with a cantilevered top shelf and a separate lower foot.

![Servo pocket, viewed down the joint axis](hardware/cad/comparisons/coax-servo-pocket.png)

Looking straight down the joint axis makes the point cleanly: **same five-hole horn
pattern, different pocket.** The original wraps the servo in a rounded-square shell
sized to its case; mine is larger, squarer, corner-relieved, and offset from the horn
centreline.

### Femur

![Original vs modified femur](hardware/cad/comparisons/femur-comparison.png)

Same story at the other end of the leg. The hip-end horn hub is preserved; the knee end
goes from a slim slotted fork to a flat mounting pad with square posts and a boxed
section that captures the replacement knee servo. The part ends up **24 mm wider and
59% heavier in material terms** — that is the honest cost of housing a servo the
original geometry was never drawn around.

### Lower leg

![Original vs modified lower leg](hardware/cad/comparisons/tibia-comparison.png)

→ **Full walkthrough, including how the parts were modelled and what is still open:
[`docs/mechanical-redesign.md`](docs/mechanical-redesign.md)**

---

## Electronics

The build photo at the top is the best single piece of evidence: populated driver board,
buck converter, XT60 power connector, twelve servos, speaker, and hand-labelled wiring
looms.

What I did: assembled and soldered the boards, worked out a power distribution scheme
around the parts I could actually buy, and brought each subsystem up **one at a time**
rather than flashing the full firmware and guessing. That last decision is the one that
saved the most time, and it is why [`Code/Tests/`](Nova-SM3/Code/Tests/) exists — eight
standalone sketches, one per peripheral, each with an explicit pass criterion:

| Sketch | Board | Proves |
|---|---|---|
| `Test_PWM_Servos` | Teensy 4.0 | PCA9685 @ 0x40, 12 servos, OE pin |
| `Test_PS2` | Teensy 4.0 | Controller link |
| `Test_MPU6050` | Teensy 4.0 | IMU on `Wire1` |
| `Test_PIR` / `Test_Ultrasonic` | Teensy / Nano | Motion and range sensing |
| `Test_OLED` / `Test_NeoPixel` | Nano | Display and RGB eyes |
| `Test_MP3` | Teensy 4.0 | DFPlayer Mini |

A pass on these proves *wiring and hardware*, not firmware — which is exactly what you
want when you are trying to find out whether a fault is in your soldering or in
somebody else's code.

One concrete example of what bring-up caught, now written into the source as a warning:
**the PS2 receiver is a 3.3 V part and the Teensy 4.0 is not 5 V tolerant.** Powering
the receiver from 5 V and wiring DAT back to the Teensy damages the microcontroller
through the data line.

→ **Details, including what is and isn't documented:
[`docs/electronics.md`](docs/electronics.md)**

---

## Firmware

The inherited firmware worked but was hard to change: the Teensy master was a single
**6,390-line `.ino`**, with an 812-line `ps2_check()` nested eight levels deep, a
425-line `setup()`, and 85 undocumented magic strings like `"MQNOFzn"` as the I²C
protocol between the two boards.

I produced **v6.0**: the same robot, reorganised.

| | v5.1 (inherited) | v6.0 (mine) |
|---|---|---|
| Teensy sketch | one 6,390-line file | 12 topical tabs, largest 886 lines |
| Largest function | `ps2_check()` — 812 lines, 8 deep | `follow()` — 215 lines |
| `setup()` | 425 lines | 19 lines, one call per boot step |
| Slave protocol | 85 undocumented strings | named constants in a shared header |
| Nano flash used | 91% | 83% |

Behaviour is deliberately unchanged, and that was verified rather than assumed: the
slave-protocol renaming and the file split both produced **byte-identical compiler
output**. Two genuine defects surfaced while reading and were fixed — a button-release
handler that tested `PSB_R2` where it meant `PSB_R1` (so releasing R1 never stopped the
body roll), and three ultrasonic debug prints where only the first was behind its debug
guard.

**On AI assistance:** the v6.0 restructure was done with Claude Code, and I have said so
in the firmware README, in the source headers, and here. The refactor was mine to
direct, review and verify; I am not going to pretend otherwise in either direction.

**On the controller:** I did *not* replace an nRF system with PS2. It is the other way
round — Chris Locke's **newer** versions moved to an nRF module with a custom-built
remote, and his older versions used a PS2 controller. I chose to build on the older PS2
path rather than fabricate a bespoke remote, so v6.0 is, as my own source comment puts
it, a "Frankenstein" — v5.1's structure with the PS2 control path retained.

→ **Full changelog and architecture: [`Nova-SM3/Code/README.md`](Nova-SM3/Code/README.md)**

---

## Status

Honest state of the build:

| | |
|---|---|
| Mechanical design and printing | ✅ Complete |
| Assembly | ✅ Complete |
| Electronics integration | ✅ Complete |
| Per-subsystem bench tests | ✅ Passing |
| PS2 controller → servo path | ✅ Verified |
| Firmware v6.0 restructure | ✅ Complete, compiles clean for both target boards |
| **Servo recalibration for my servos** | ❌ **Not done** |
| **Walking gaits on this robot** | ❌ **Not working yet** |

The last two are the same problem, and it is worth being clear about it because it is
the most interesting open item in the project:

`NovaServos.h` still contains **Chris Locke's `servoHome[]` and `servoLimit[]` values,
byte-for-byte**. Those numbers are physical measurements of *his* robot — raw PWM ticks
for his servos in his leg geometry. Mine has different servos in legs I redesigned, so
those values do not describe this machine. The gaits are hand-tuned against them, which
is why locomotion is the piece that does not work yet. The fix is not a code change: it
is bench work with [`Nova-SM3-calibrate/`](Nova-SM3/Code/Nova-SM3-calibrate/) to
re-measure home and limit positions for all twelve joints, and then re-tuning from
there.

---

## Repository layout

```text
README.md                       this file
docs/
  mechanical-redesign.md        how the leg parts were modified, and why
  electronics.md                power, wiring, bring-up, and what is not documented
hardware/
  cad/
    original/                   Chris Locke's unmodified reference STLs
    modified/                   my geometry — STL + native SolidWorks parts
    comparisons/                rendered before/after figures
    README.md                   which file is whose, and measured dimensions
  electronics/                  schematics and wiring
  media/                        build photography
Nova-SM3/Code/                  the firmware (see its own README)
```

**`hardware/cad/original/` is never overwritten.** Keeping Chris Locke's geometry
intact next to mine is the only way either the attribution or the comparison means
anything.

---

## Credit and licence

**Original project — Chris Locke** (`cguweb@gmail.com`) ·
[novaspotmicro.com](https://novaspotmicro.com) ·
[GitHub](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3) ·
[Thingiverse](https://www.thingiverse.com/thing:4767006) ·
[Instructables](https://www.instructables.com/Nova-Spot-Micro-a-Spot-Mini-Clone/).
MIT licensed — see [LICENSE](LICENSE), Copyright (c) 2021 Christopher M. Locke.

**Third-party parts** — Thingiverse user **Ozzymat**'s
[Nova SM3 electronics-mounting remix](https://www.thingiverse.com/thing:7131451)
(CC BY-SA) is in my working files as a reference for mounting the PCA9685 and the
XL6009 buck converter. Those parts are **not** included in this repository and are
**not** my work; the geometry documented here as mine is the hip, femur and tibia only.

**Modifications, mechanical redesign, electronics and v6.0 firmware — Jeremy Aidan Yu**,
with the v6.0 restructure done with assistance from Claude Code.
