# Electronics: wiring a reference design that had gone stale

← [back to the main README](../README.md)

---

## What is actually inside

Before any diagrams, this is the real thing:

![Electronics bay, installed](media/electronics-bay-installed.jpg)

*Looking down into the open electronics bay of the assembled robot. The controller PCB is
at the bottom of the frame with the servo harnesses running out to the legs, and the
step-down converter with its toroidal inductor and finned heatsinks sits above it.*

<table>
<tr>
<td width="50%"><img src="media/controller-pcb-populated.jpg" alt="Controller PCB populated"></td>
<td width="50%"><img src="media/electronics-bay-detail.jpg" alt="Electronics bay detail"></td>
</tr>
<tr>
<td colspan="2"><em>Left: the Nova SM3 controller board after I populated and soldered it,
before installation. It is the project's own PCB, not an off-the-shelf module: JST servo
headers, an Arduino Nano, an MPU6050 breakout, a DIP switch and screw terminals. Right: the
same board fitted in the chassis beside the converter. The masking-tape labels reading
<code>+VCC / SDA / SCL / OE / GND</code> are mine, written during bring-up so the I2C and
output-enable lines could be traced against the drawing without counting pins every
time.</em></td>
</tr>
</table>

**What has to be powered:** twelve servos, two microcontrollers, an IMU, three PIR sensors,
two ultrasonic sensors, an OLED, four addressable RGB LEDs, an MP3 player and a speaker,
all from one LiPo pack. Servos dominate, and the transient when a gait starts is the worst
case. That is the constraint everything else has to survive.

---

## The reference design had gone stale

This is the part of the project I would most want someone to look at.

The published Nova SM3 wiring documentation is for an **older, deprecated revision** of the
project. It does not describe the hardware as it actually is, and following it as drawn
does not give you a working robot. I did not discover that by reading. I discovered it by
printing the drawings out and tracing every net by hand against the real board.

![Schematic used as a continuity checklist](wiring/schematic-continuity-check.jpg)

*Chris Locke's REV 5.1 schematic, printed and used as a live checklist. Blue ticks are nets
I confirmed with a multimeter. Green highlighter follows the ones I was chasing. The
PS2-COM header is hand-labelled DAT / CMD / SEL / CLK because that mapping had to be worked
out. Bottom right, `cont gnd` with a tick: ground continuity confirmed. Top right, next to
the GND and 6.8V-VIN terminal blocks, the note that mattered:* **`Shorted!`** *, and beside
it, `short? i think so... (es :)`.*

That annotation is the moment the investigation turned. Working through the drawing node by
node, the terminal arrangement it specified did not agree with the hardware, and taking it
literally put rails together that must not touch. The errors were in the documentation, not
in the build.

The same treatment was applied to the pictogram:

![Pictogram traced by hand](wiring/wiring-diagram-annotated-by-hand.jpg)

*The v5.2b pictogram, traced in highlighter along the power path with pen notes throughout:
`6.8 in` and `5.4 out` at the XL6009, `No D23!` where a pin does not exist, circles around
the PIR sensors, the RGB dimmer pot and the OLED power, and the PIR pin question
`D6 - Right, D5 - Left, D4 - Front ?`.*

That last question resolved itself in code: `Test_PIR.ino` defines `PIR_FRONT 4`,
`PIR_LEFT 5`, `PIR_RIGHT 6`, so the pencilled guess was right.

The output of all of this tracing is a **corrected drawing**, which carries its own revision
block:

![Revision block](wiring/wiring-revision-notes.png)

→ **Full diagram: [`wiring/wiring-diagram-modified.png`](wiring/wiring-diagram-modified.png)**
(6000 x 4712, open it full size to read pin labels)

The base artwork is Chris Locke's. The revisions and the revision block are mine. It is
committed as the drawing itself rather than redrawn or summarised, so the documentation and
the artifact cannot drift apart the way the upstream pair did.

---

## Revision 1: cascaded converters reworked to parallel

**What the reference design did.** The XL6009 5.4 V converter took its input from the
**6.8 V converter's output block**. The two converters therefore ran **in series**.

**Why that is wrong here.** Everything downstream of the XL6009 inherits the 6.8 V stage's
droop under load and pays two sets of conversion losses. With twelve servos pulling hard on
the same pack, that is precisely the arrangement that turns a servo current transient into
a brownout on the logic rails.

**The fix.** I moved the XL6009 input **off** the 6.8 V output block and onto the switched
battery pair at the 6.8 V buck input. **Both converters now run in parallel off the pack**,
so neither sees the other's output impedance. The rework also freed one VIN and one GND
terminal.

> Revision 1, in the drawing's own words: *"XL6009 5.4V converter: input moved OFF the 6.8v
> Output 'VIN' block. +IN / -IN now tap the switched battery pair at the 6.8V buck input, so
> both converters run in PARALLEL off the pack instead of cascaded. One VIN and one GND
> terminal freed."*

## Revision 2: a ground that dead-ended on a power rail

While redrawing the sensor wiring I found that **the PIR ground drop terminated on the
`VIN PIR, PS2, SW` +5 V rail** instead of the ground rail.

A ground return landing on a power rail is not subtle in its effects, but it is very easy
to miss on a dense pictogram, because the two rails run parallel and that crossing looks
like every other crossing on the sheet.

So I fixed both the fault and the reason it was missable: the ground drop now lands on the
GND rail, and **the four remaining +5 V / GND crossings were redrawn as explicit
over-under**, with a white break showing the wire passing in front. On the corrected drawing
a crossing can no longer be read as a junction.

## Revisions 3 to 5: component substitutions

| Original | What I fitted | Consequence |
|---|---|---|
| NRF24L01 plus a custom-built remote | **Yahboom 2.4 G PS2 receiver** | On the PS2-COM header: MISO-DAT-D7, MOSI-CMD-D8, CS-SEL-D9, SCK-CLK-D10, ACK n/c, **VCC on 3.3 V**, GND2. The D13 (SCK) drop and the NRF 22 uF cap are deleted. D9/D10 are shared with the NRF footprint, **so that footprint must stay unpopulated.** |
| DFPlayer Mini | **DFPlayer PRO (DFR0768)**, 3.3 to 5 V | Speaker moved to R+ / R- (right channel). VIN / GND / RX / TX unchanged; left channel, DAC and button pins unused. |

The receiver itself had nowhere to live, since the body was designed around a radio
soldered to the board. It ended up in a backpack borrowed from the Nova SM2, mounted on a
new back panel so it protrudes from the body like a tail and keeps its antenna outside the
electronics bay. That is documented on the
[mechanical page](../mechanical/README.md#the-tail-borrowing-a-backpack-from-the-nova-sm2).

Revision 5 is a caveat I put on the drawing rather than a change: the artwork for the PS2
receiver and the DFR0768 is **drawn, not photographic**, so check the vendor pinout before
wiring.

The PS2 substitution is also why the firmware runs on the older PS2 code path rather than
upstream's newer nRF path. See the [firmware write-up](../firmware/README.md).

---

## Bring-up: one subsystem at a time

The decision that mattered most was **not** to assemble everything, flash the full
firmware, and start guessing.

The inherited firmware is large, it drives a lot of peripherals, and a fault could be in my
soldering, my wiring, my power rail, or somebody else's code. Debugging all of those at once
is how you lose a week. Given that the reference documentation had already proven unreliable,
assuming any of it was correct would have been a mistake.

So I wrote [`firmware/Tests/`](../firmware/Tests/): eight standalone sketches, one per
peripheral, each of which deliberately includes **none** of the main firmware. A pass proves
wiring and hardware, not the main sketch.

| Sketch | Board | Under test | Pins, as defined in the sketch |
|---|---|---|---|
| `Test_PWM_Servos` | Teensy 4.0 | PCA9685 and 12 servos | `OE_PIN 3`, jogged over PS2 |
| `Test_PS2` | Teensy 4.0 | PS2 receiver | DAT 7, CMD 8, SEL 9, CLK 10 |
| `Test_MPU6050` | Teensy 4.0 | IMU | SDA2 17, SCL2 16 |
| `Test_PIR` | Teensy 4.0 | 3x PIR | front 4, left 5, right 6 |
| `Test_MP3` | Teensy 4.0 | DFPlayer | volume pot 20 |
| `Test_Ultrasonic` | Arduino Nano | 2x HC-SR04 | L trig 7 / echo 6, R trig 5 / echo 4 |
| `Test_OLED` | Arduino Nano | SSD1331 96x64 | active pin 12 |
| `Test_NeoPixel` | Arduino Nano | 4x WS2812 | data 2, brightness pot A3 |

Each carries an explicit pass criterion, so "it did something" does not get mistaken for
"it works".

![PS2 bring-up on the bench](media/ps2-bringup-bench.jpg)

*The bring-up rig: converter, controller board and jumper harness on the bench, driven from
a laptop. Buttons and stick positions were checked against live serial output before the
receiver was trusted anywhere near the assembled robot.*

---

## Hazards worth knowing

These are in the source as warnings because they were live problems, not hypotheticals.

**The Teensy 4.0 is not 5 V tolerant.** From
[`Test_PS2.ino`](../firmware/Tests/Test_PS2/Test_PS2.ino): *"The PS2 receiver is a 3.3V
part. The Teensy 4.0 is ALSO 3.3V and is NOT 5V tolerant, do not feed the receiver 5V and
then wire DAT back to the Teensy."* The failure mode is nasty because the receiver **works**
on 5 V. It just quietly destroys the microcontroller through the data line. The corrected
drawing accordingly puts the receiver's VCC on 3.3 V.

**`OE_PIN` is active LOW, and boot order matters.** Driving OE low *enables* the servos. It
is held high through boot until the PS2 link is up, because the PWM driver interferes with
the receiver; without that sequencing the robot lurches on power-up. Servo output arms about
a second after boot.

**Current sense and battery sense share one pin.** `AMP_PIN` and `BATT_MONITOR` are both
**A1**. The robot physically cannot measure servo current and battery voltage at the same
time. This is inherited from the original design and is an architectural limitation, not a
bug.

**Debug flags block an untethered boot.** On a Teensy, printing to a serial port with
nothing attached stalls the board, so any debug flag left set in `NovaConfig.h` makes the
robot appear completely dead the moment you unplug the laptop.

---

## What is not documented here

- **No measurements recorded.** No current draw figures, no rail voltages under load, no
  scope captures. The reasoning above and the corrected drawing are the evidence; I did not
  log the readings that led me there and I am not going to invent them after the fact.
- **No continuity log.** The tracing happened, and the annotated printouts above are what
  survives of it. The individual readings were not written down.
- **No bill of materials** for the substituted parts beyond what the drawing names.
