# Nova SM3: Firmware

Nova SM3 is the firmware for a **Spot-Micro-class quadruped robot**: a twelve-servo
four-legged walker built around a Teensy 4.0 and an Arduino Nano. It walks, trots,
marches, sits, kneels, follows a person around a room using PIR sensors, and reports
what it is doing on a small colour display and a pair of RGB "eyes".

This directory holds the Arduino sketches. The STL files, wiring diagrams and parts
list live one level up in `../`. The project's home is
[novaspotmicro.com](https://novaspotmicro.com).

---

## What Version 6.0 is

**Version 6.0 is a readability release.** It is version 5.1 reorganised and documented,
not rewritten. The robot behaves the same, the servo calibration is byte-for-byte
identical, and the gait tuning is untouched.

What changed is how the code reads:

| | v5.1 | v6.0 |
|---|---|---|
| Teensy sketch | one 6,390-line `.ino` | 12 tabs, largest 886 lines |
| Largest function | `ps2_check()`, 812 lines, 8 levels deep | `follow()`, 215 lines |
| `serial_command()` | 633 lines | 18-line dispatcher + 10 topic groups |
| `setup()` | 425 lines | 19 lines, one call per boot step |
| Nano `requestCallback()` | 264 lines | 27 lines + 2 handlers |
| Slave protocol | 85 undocumented strings like `"MQNOFzn"` | named constants in a shared header |
| Nano flash used | 91% | 83% |

Two genuine bugs were found and fixed along the way; both are listed under
[Behaviour changes](#behaviour-changes) and marked `BUGFIX (v6.0)` in the source.

Version 5.1 is left in place as `Nova-SM3_teensy-v5.1/` and `Nova-SM3_nano-v5.1/`.
If something misbehaves, those are the reference. Note that v5.1 already carries the
PS2 diagnostic reporter (`ps2_debug_report()` and the `ps2` / `ps2r` serial commands)
added during earlier bring-up work; v6.0 inherits it.

---

## The two boards

Nova runs on two microcontrollers connected over i²c.

```
        ┌──────────────────────────┐                ┌────────────────────────┐
        │  Teensy 4.0  -  MASTER   │      i²c       │  Arduino Nano - SLAVE  │
        │                          │◄──────────────►│                        │
        │  12 servos (PCA9685)     │  one command   │  SSD1331 OLED          │
        │  MPU6050 IMU             │  byte at a     │  4 NeoPixel "eyes"     │
        │  PS2 wireless remote     │  time          │  2 ultrasonic sensors  │
        │  3 PIR motion sensors    │                │  EEPROM settings       │
        │  current + battery sense │                │                        │
        │  DFPlayer Mini MP3       │                │                        │
        │  USB serial commands     │                │                        │
        └──────────────────────────┘                └────────────────────────┘
             does all the thinking                    owns only its own I/O
```

The Teensy makes every decision. The Nano makes none, it receives single-byte
commands and carries them out. **Every one of those bytes is named and documented in
`NovaSlaveProtocol.h`,** which is the single most useful file to read if you want to
understand how the two halves fit together.

The two sketches must be flashed as a matched pair.

---

## Directory layout

```
Code/
├── README.md                     this file
├── CLAUDE.md                     notes for AI coding assistants
│
├── Nova-SM3_teensy-v6.0/         ← CURRENT master sketch (Teensy 4.0)
├── Nova-SM3_nano-v6.0/           ← CURRENT slave sketch  (Arduino Nano)
│
├── Nova-SM3_teensy-v5.1/         previous master, kept as reference
├── Nova-SM3_nano-v5.1/           previous slave, kept as reference
├── Nova-SM3_teensy-v5.0/         older master (no MP3 support)
├── Nova-SM3_teensy-v4.2/         Mega-era master
├── Nova-SM3_nano-v4.2/           Mega-era slave
│
├── Nova-SM3-calibrate/           bench sketch: measure servo home + limits
├── Tests/                        one small sketch per subsystem, for bring-up
└── Arduino Libraries/            vendored third-party libraries
```

Each version directory is a **complete, independent sketch**, not a patch on the one
before it. They are snapshots. A fix made in v6.0 does not propagate anywhere.

---

## Master sketch: `Nova-SM3_teensy-v6.0/`

Open `Nova-SM3_teensy-v6.0.ino` in the Arduino IDE and the rest appear as tabs.
They all compile together as one program, so a global declared in the main tab is
visible in every other one.

| File | Responsibility |
|---|---|
| `Nova-SM3_teensy-v6.0.ino` | **Start here.** Globals, `setup()`, `loop()`, the boot sequence, shutdown |
| `NovaConfig.h` | **The file you edit.** Debug flags, which hardware is fitted, the pin map |
| `NovaServos.h` | Per-robot servo calibration and the motion state arrays |
| `NovaSlaveProtocol.h` | Names for every byte sent to the Nano: shared with the slave sketch |
| `AsyncServo.h` | The non-blocking servo motion engine |
| `MPU6050_conf.h` | Register-level IMU driver (third-party, left as-is) |
| `pitches.h` | Musical note frequencies (stock Arduino file) |
| `Ps2Remote.ino` | Reading the remote and turning it into movement requests |
| `Sensors.ino` | PIR, ultrasonic, IMU, current draw, battery voltage |
| `ServoControl.ino` | The layer between movement routines and `AsyncServo` |
| `Poses.ino` | Named stances: stay, sit, crouch, lay, kneel, look left/right |
| `Motion.ino` | Body attitude: roll, pitch, yaw, axis wiggles |
| `Gaits.ino` | The walking gaits |
| `Sequences.ino` | Scripted multi-step routines and the sequencer |
| `SerialCommands.ino` | The typed command interface |
| `SlaveComms.ino` | The i²c conversation with the Nano |
| `Sound.ino` | Buzzer tunes and MP3 playback |
| `Util.ino` | Clamping, unit conversion, joint predicates |

### Where the main logic lives

Everything hangs off `loop()`, which does exactly three things on every pass and
**never blocks**:

```
loop()
 │
 ├─ 1. update_servos()          advance all 12 servos one PWM tick, if due
 │                              → AsyncServo::Update()  (AsyncServo.h)
 │
 ├─ 2. one movement routine     a chain of move_* flags selects AT MOST ONE
 │                              → step_march(), roll(), wake(), …
 │                                (Gaits.ino, Motion.ino, Sequences.ino)
 │
 └─ 3. poll each subsystem      each on its own millis() timer
                                → ps2_check(), serial_check(), pir_check(),
                                  get_mpu(), uss_check(), battery_check()
```

The single most important thing to understand about this codebase:

> **Movement routines do not move anything.** They set `targetPos[]` and
> `servoSpeed[]` for the servos they care about, and return immediately. The motion
> happens later, one tick at a time, inside `AsyncServo::Update()`.

That is why a routine that "does nothing" is almost always a movement flag that was
never set, or one that something else already cleared. It is also why nothing in the
sketch is allowed to call `delay()` while the robot is running.

### The four PS2 button sets

The remote has four modes, cycled with SELECT and tracked in `ps2_select`. Every
button means something different in each. If a control seems dead, check which set is
active first, this catches most "the remote is broken" reports.

| Set | Purpose |
|---|---|
| 1 | Walking and body attitude: roll, pitch, axis wiggles |
| 2 | Gait control and fixed poses: sit, kneel, crouch, lay |
| 3 | Kinematics: the sticks steer body position and yaw directly |
| 4 | Per-joint calibration jogging, for bench work |

Type `ps2` on the serial monitor to get a live report of button presses, stick values
and the current set.

---

## Slave sketch: `Nova-SM3_nano-v6.0/`

| File | Responsibility |
|---|---|
| `Nova-SM3_nano-v6.0.ino` | **Start here.** Globals, `setup()`, `loop()`, the two i²c handlers |
| `NovaSlaveProtocol.h` | The command bytes: **must stay identical to the master's copy** |
| `NovaBitmap.h` | The boot logo bitmap |
| `Display.ino` | Everything drawn on the OLED |
| `Leds.ino` | The NeoPixel eye animations |
| `Settings.ino` | EEPROM settings and the self-reset |

Two i²c interrupt handlers do all the work: `receiveEvent()` stores the incoming
byte, `requestCallback()` acts on it and returns a response. `loop()` only advances
the LED animation and the display, and watches the front-panel button.

The Nano keeps one piece of state that changes what a byte means, `serial_oled`.
When it is 0 an incoming byte is a system command (LEDs, sensors); when it is 1 it is
a display command. The byte `'X'` toggles it. On the master you never toggle it by
hand: use `rgb_request()` and `oled_request()`, which handle it for you.

---

## Building and flashing

There is no build system, test suite, or linter. These are plain Arduino sketches
and validation is physical, flash them and exercise the robot.

### Arduino IDE

1. Install the **Teensyduino** add-on for the Teensy 4.0.
2. Point the sketchbook library path at `Arduino Libraries/`, or install the
   equivalents through Library Manager.
3. Install **DFRobotDFPlayerMini** through Library Manager. It is *not* vendored.
   (`Arduino Libraries/DFRobot_DF1201S.zip` is for the DFPlayer Mini **Pro**, a
   different module with a different API.)
4. Open the sketch, select the board, Verify, Upload.

### arduino-cli

```bash
# master
arduino-cli compile --fqbn teensy:avr:teensy40 \
    --libraries "Arduino Libraries" Nova-SM3_teensy-v6.0

# slave
arduino-cli compile --fqbn arduino:avr:nano \
    --libraries "Arduino Libraries" Nova-SM3_nano-v6.0
```

Both compile clean as of this release. Current usage:

* **Teensy 4.0**: 121 KB flash of 1.9 MB available, 29 KB RAM1. Plenty of room.
* **Arduino Nano**: 83% of flash, 48% of RAM. **Very little room.** Check the
  compiler's final report before adding anything to the slave.

---

## Things you need to know before running it

**Set every debug flag in `NovaConfig.h` to 0 before running Nova untethered.**
On a Teensy, printing to a serial port that nothing is listening on will stall the
board, and Nova will appear completely dead. This is the single most common way to
lose an afternoon on this project.

**The Teensy 4.0 is not 5V tolerant.** Every input must stay at or below 3.3V. In
particular the PS2 receiver must be powered from 3.3V, powering it from 5V will
damage the Teensy through its DATA line.

**`OE_PIN` is active LOW.** Driving it LOW *enables* the servos. It is held HIGH
through boot until the PS2 link is up, because the PWM driver interferes with the
receiver and would otherwise make the robot lurch on power-up. Servo output arms
roughly a second after boot.

**`servoHome[]` and `servoLimit[]` in `NovaServos.h` are physical measurements** of
one assembled robot, in raw PWM ticks. They are not tunable constants. If a leg sits
wrong, re-measure with `Nova-SM3-calibrate/` rather than nudging the numbers. They
are only meaningful at the `SERVO_FREQ` / `OSCIL_FREQ` set in `NovaConfig.h`, which
is why both carry a "do not change once calibrated" warning.

**`servoLimit[]` pairs are ordered by leg direction, not numerically.** For the left
legs the first value is the *larger* number, because those servos are mirrored. Any
code comparing against them has to check which way round the pair is, see
`limit_target()`.

**Speed is a delay.** `servoSpeed[]` is the number of milliseconds between one PWM
tick and the next, so a *bigger* number means a *slower* servo. This is why
`max_spd` (1.0) is numerically smaller than `min_spd` (96.0).

**`AMP_PIN` and `BATT_MONITOR` are the same analog pin (A1).** This robot cannot
read current and battery voltage at the same time. Do not enable both `amp_active`
and `batt_active` without rewiring.

**`NovaSlaveProtocol.h` exists twice**, once in each sketch folder. The Arduino IDE
cannot share a header between two sketch folders. The two copies must be kept
identical or the boards will disagree about what a byte means.

---

## Author / Modification Notes

**Original author: Chris Locke** (<cguweb@gmail.com>), Nova SM3 is his design and
his code, including the mechanical design, the gait development, the servo motion
engine and the master/slave architecture. Project home:
[novaspotmicro.com](https://novaspotmicro.com) ·
[GitHub](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3) ·
[Thingiverse](https://www.thingiverse.com/thing:4767006) ·
[Instructables](https://www.instructables.com/Nova-Spot-Micro-a-Spot-Mini-Clone/)

**Version 6.0 modified by: Jeremy Aidan Yu**, with assistance from **Claude Code** (Anthropic).

Version 6.0 makes no claim on the design or the behaviour of the robot. The work in
this release is confined to structure, naming and documentation. The original code
was written as a personal project under active hardware development, with the
author's own notes and open questions left in place deliberately, those notes are
preserved here, and the DEV NOTE comments marking unfinished work are still in the
source where he left them.

---

## What changed in Version 6.0, and why

Every change below is a real edit, verified by compiling both sketches for their
actual target boards after each step. Where a change was purely mechanical, the
compiler's reported flash and RAM usage was compared before and after: the slave
protocol renaming and the file split both produced **byte-identical output**, which
is about as strong a guarantee as is available without hardware.

### Organisation

**Split the 6,390-line master sketch into 12 tabs by topic.** A single file that
large cannot be navigated; you scroll for it. The tabs follow the section banners
the original author had already put in the file, so the grouping is his, not an
invention. Arduino compiles all tabs as one program, so this changed nothing about
how the code works, and was verified by identical compiler output before and after.

**Split the 1,265-line slave sketch into four source files and two headers**, and
moved the 64-line boot logo
out of the middle of the variable declarations into `NovaBitmap.h`.

**Extracted `NovaConfig.h`.** Debug flags, "what hardware is fitted" flags and every
pin `#define` were scattered through 380 lines of globals. They are now in one file
with a full pin-map table, because they are the only things most people ever edit.

**Broke up the three functions that were 29% of the file.**

* `ps2_check()` (812 lines, eight levels of nesting, 57 tests of `ps2_select`) is
  now a 25-line dispatcher plus one handler per button group. The structure was
  always *[which button] × [which of four sets]*; it was just flattened into one
  chain.
* `serial_command()` (633 lines of chained string comparison) is now an 18-line
  dispatcher over ten topic groups, each returning whether it recognised the
  command. First-match-wins ordering is preserved exactly.
* `setup()` (425 lines) is now 19 lines, one call per boot step, so the order the
  hardware comes up in is readable at a glance.

**Split the slave's `requestCallback()`** (264 lines) into an OLED-command handler
and a system-command handler.

### Naming and magic values

**Named every byte of the master/slave protocol.** This was the single biggest
obstacle to understanding the codebase. Calls looked like this:

```cpp
rgb_request((char*)"MONRDn");     // ...meaning what?
oled_request((char*)"9n");
```

and could only be decoded by reverse-engineering a 264-line switch on the other
board. There are now named constants in `NovaSlaveProtocol.h`, shared by both
sketches, and because they are string literals they concatenate at compile time, so
the call sites cost exactly the same flash they did before:

```cpp
rgb_request(RGB_LEFT RGB_RED  RGB_RIGHT RGB_YELLOW  RGB_SPEED_30 RGB_PATTERN_BLINK);
oled_request(OLED_BATTERY_LEVEL_9 OLED_BATTERY_GAUGE);
```

All 125 call sites were converted by script rather than by hand, so no string was
mistranscribed.

**Named the columns of the multi-dimensional motion arrays.** `servoRamp[id][5]` and
`servoSweep[id][2]` are now `servoRamp[id][RAMP_DOWN_SPEED]` and
`servoSweep[id][SWEEP_DIR]`. Eight ramp columns and five sweep columns were
previously bare indexes that had to be traced through `AsyncServo` to interpret.

**Replaced ASCII codes with the characters they stand for.** The slave's display
dispatch was written as `if (cmd == 97) { //a` … `else if (cmd == 119) { //w`, so the
reader had to trust 23 hand-written comments. It now compares against the protocol
constants directly.

**Renamed `AsyncServo::pwmBoard` to `pwmPin`.** It holds a PCA9685 *channel* number,
not a board id, and it carried a copy-pasted comment claiming it was the pulse
increment. Three things with three different meanings under one name.

**Corrected several comments that described something other than what the code did**,
including the `servoSweep` column list (it named a nonexistent "sweep type" and
described a cycle counter as milliseconds) and the note about `pinMode` ordering in
the servo test sketch.

### Removing duplication

**Extracted `set_sweep()`.** The seven-line block that arms a servo to sweep between
two positions appeared **67 times**. It is now one documented function and 67 call
sites, which took about 260 lines of near-identical code out of the movement
routines and makes the actual differences between gaits visible.

**Extracted the ramp interpolation in `AsyncServo`.** A 35-line block was duplicated
character-for-character between the move path and the sweep path, and was verified identical
before extracting, so a future fix to one can no longer miss the other.

**Collapsed the PS2 joint-jogging code.** Twelve copies of an 11-line "nudge this
joint by one tick" block became twelve one-line calls to `ps2_jog_joint()`.

**Table-driven the slave's command decoding.** The RGB commands arrive as four
contiguous blocks of bytes (pattern, repeat count, step interval, colour), so each
block is now a range test and a small `PROGMEM` lookup table instead of a `case` per
byte. Eight near-identical colour cases became one.

**Table-driven the battery gauge.** 111 lines of `fillRect` calls across ten
if/else branches were, on inspection, a 10×5 table of bar heights and colours where
every bar sits on the same baseline. It is now that table plus a five-iteration loop.
The ten voltage comparisons it used to make all resolved to "which level is it",
since the voltage being compared came from the same array, so the level indexes the
table directly.

**Table-driven the radar grid.** 32 hand-written `drawLine` calls became two short
loops over the gap positions.

**Table-driven the boot report.** 84 lines of "print a name, toggle the LED, wait"
became a table of subsystems and two loops.

Together these are the reason the Nano's flash use fell from **91% to 83%**. The
duplication was real, and it was costing space on the board that had none to spare.

### Documentation

**Rewrote the header of every file** to say what the file is for, what board it
targets, and how it fits with the others.

**Documented the things that surprise people**, at the point where they surprise
them: that speed is a delay so bigger is slower; that `OE_PIN` is active low; that
`servoLimit` pairs are direction-ordered rather than numerically ordered; that the
PWM channels skip 3, 7, 11 and 15 so the servo index is not the channel number; that
`AMP_PIN` and `BATT_MONITOR` are the same physical pin; that any debug flag left set
will stop the Teensy booting untethered.

**Explained the `plotter` flag.** Roughly 90 `Serial.print` calls were wrapped in
`if (!plotter)` with no explanation anywhere in the project. It suppresses
human-readable output so the remaining numeric output can be fed to the Arduino
Serial Plotter, which only understands `label:value` lines.

**Rewrote the serial help screen.** The old one was hand-aligned with tab
characters and had drifted out of step with the code: `ms+` and `ms-` were described
with each other's behaviour, a row labelled `s-` actually documented `l-`, and two
commands (`stl`, `str`) were listed that do not exist. It is now generated from a
table grouped to match the command handlers, and lists everything the sketch accepts.

### Dead and confusing code

* Removed a `test_loops` soak-test block wired into the middle of `step_march()`.
  Its trigger variable was initialised to 0 and never set anywhere, so it could not
  run; it also carried its own broken indentation.
* Removed eleven global variables that were declared and never read. Two of them,
  `pattern_int` and `pattern_cnt`, shadowed the names of variables on the *slave*
  that are load-bearing, which is actively misleading.
* Removed `save_ep_data()` on the slave, which was declared, defined and never
  called. The EEPROM write it duplicated still happens inline in the reboot
  handlers, and `load_ep_data()` now documents why the other four settings are read
  but never written.
* Removed a system-mode `'c'` command on the slave that set `oled_command = 9997`.
  The value was truncated to a meaningless byte on the way to `oled_check()`, and
  the master never sends `'c'` in that mode anyway.
* Simplified a loop in `run_sequence()` that wrote `servoSequence[0]`,
  `servoSequence[l]`, `servoSequence[2]` and `servoSequence[3]` on every iteration.
  It cleared all four legs by accident; clearing `servoSequence[l]` does the same
  thing on purpose.
* Removed empty `if` branches that existed only to be fallen through, and inverted
  one `if (…) { } else { … }` that read backwards.
* Renamed an inner loop variable that shadowed the outer sequence index in
  `delay_sequences()`.
* Reordered `delay_sequences()`'s sixteen branches into numeric order and made them
  a `switch`. Routine 13 was previously listed between 2 and 3, which is easy to
  miss when you are looking for it. This also removed a dangling `if (debug1)` whose
  body had been commented out, leaving it silently attached to the next statement.

### Behaviour changes

Two, both fixes for real defects found while reading:

1. **`Ps2Remote.ino`: releasing R1 never stopped the body roll.** The release
   handler for the R1 button tested `PSB_R2` instead of `PSB_R1`. Letting go of R1
   left `move_roll_body` set, so the body kept rolling until something else called
   `set_stop()`; letting go of R2 cleared it instead, stealing R2's own release.
   Every other trigger clears the flag its own button set. Marked
   `BUGFIX (v6.0)` in the source.

2. **`Nova-SM3_nano-v6.0.ino`: `get_distance()` printed with debugging off.** Of
   three consecutive `Serial.print` calls, only the first was guarded by `debug1`;
   the other two ran on every ultrasonic reading. All three are now inside the
   guard. Marked `BUGFIX (v6.0)` in the source.

One further, trivial difference: `serial_command()` now returns early on an empty
command instead of falling through to the "not a valid command input" message, so
typing a bare comma no longer produces an error line. The old test was `if (cmd)`,
which on an Arduino `String` is always true and reads as though it checks for
emptiness when it does not.

### What was deliberately left alone

* **`servoHome[]`, `servoLimit[]`, `SERVO_FREQ`, `OSCIL_FREQ`.** Physical
  calibration of one specific robot. Verified byte-for-byte identical to v5.1.
* **The gait mathematics.** `step_march()`, `step_forward()`, `wman()` and
  `funplay()` are full of hand-tuned multipliers arrived at by trial on real
  hardware. They were commented, not restructured. Renaming a variable there risks
  the tuning for no functional gain. This has no Inverse Kinematics.
* **`MPU6050_conf.h`.** Third-party register-level driver tracking a published
  datasheet. Reformatting it would make it impossible to diff against upstream. Only
  a header comment was added, explaining that and pointing at where the sensor fusion
  actually happens.
* **`pitches.h`.** The stock Arduino note table.
* **The `move_*` flag architecture.** Fifty-two booleans, of which exactly one may
  be true, is really an enum, but converting it touches every movement routine and
  every stop path at once. It is documented instead, including the warning that a new
  flag must also be cleared in `set_stop_active()`. This is the most worthwhile
  remaining change, and the right time for it is when the flags start fighting each
  other.
* **A logging wrapper for the `plotter` guard.** Replacing ~90 instances of
  `if (!plotter) Serial.println(…)` with a helper was considered and rejected: the
  guard is uniform and mechanical, and a reader only needs to learn it once. The flag
  is documented at its declaration instead.
* **Older version directories** (`v5.1`, `v5.0`, `v4.2`) and the vendored
  `Arduino Libraries/`.

### Worth reviewing on hardware

Everything here compiles for the real target boards, and the mechanical
transformations were verified against compiler output. But this is firmware, and
nothing below has been run on a robot:

* **The R1 release fix** changes what the remote does. Confirm in button set 1 that
  holding R1 rolls the body and releasing it stops.
* **The battery gauge rewrite** replaced a voltage cascade with a table lookup. The
  substitution is provable on paper, but stepping the level through 0-9 with the
  `batt` serial command is a five-minute check worth doing.
* **The `set_sweep()` extraction** touched 67 call sites across every movement
  routine. Each was converted mechanically, but exercising the sweeps (`st`, `sf`,
  `sc`, `rst`, `rsf`) and the body movements (`roll`, `pitch`, `rollb`, `pitchb`,
  `x`, `y`) covers essentially all of them.
* **The `AsyncServo` ramp extraction** is the highest-consequence change in the
  release, since that code runs for every servo on every pass. The two blocks it
  merged were verified character-identical first, but ramping is worth watching:
  `rst` and `rsf` exercise it directly.
* **The Nano's flash headroom** improved, but it remains the constrained board.
