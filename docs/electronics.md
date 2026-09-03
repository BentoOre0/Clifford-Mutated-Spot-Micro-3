# Electronics

Power, wiring, assembly, bring-up and debugging for the modified Nova SM3.

← [back to the main README](../README.md)

---

## What the robot has to power

Twelve servos, two microcontrollers, an IMU, three PIR sensors, four ultrasonic sensors,
an OLED display, four addressable RGB LEDs, an MP3 player and a speaker, from one LiPo
pack.

Servos dominate. Twelve of them stalling simultaneously is a very different load from
twelve idling, and the transient when a gait starts is the worst case. This is the
constraint the rest of the electrical design has to survive.

---

## The revised wiring diagram

The reference design is Chris Locke's v5.2b pictogram. I could not source several of the
parts it calls for, and while reworking it I also found a fault in it. The result is a
revised diagram that carries its own revision list on the drawing:

![Wiring revision notes](../hardware/electronics/wiring-revision-notes.png)

**Full diagram:**
[`Wiring_NovaSM3_v5-2b_MOD.png`](../hardware/electronics/Wiring_NovaSM3_v5-2b_MOD.png)
(6000 x 4712, open full size to read pin labels)

The base artwork is Chris Locke's. The revisions, and the revision block, are mine. It is
committed as the drawing itself rather than redrawn or summarised, so what you read here
and what is on the drawing cannot drift apart.

---

## Debugging: cascaded converters

**Observed.** The power tree misbehaved under load in a way consistent with rail sag
rather than a dead short, and the layout put two switching converters in series.

**Investigation.** Tracing the power path on the reference drawing rather than guessing
from behaviour. Continuity and resistance checks between schematic nodes to confirm what
was physically connected to what.

**Root cause.** The XL6009 5.4 V converter took its input from the **6.8 V converter's
output block**. The two converters were therefore **cascaded**: everything downstream of
the XL6009 inherited the 6.8 V stage's droop under load, and paid two sets of conversion
losses. With twelve servos drawing hard on the same pack, that is precisely the
arrangement to avoid, and it is also the arrangement that turns a servo current
transient into a brownout on logic rails.

**Fix.** I moved the XL6009 input **off** the 6.8 V output block and onto the switched
battery pair at the 6.8 V buck input. Both converters now run **in parallel off the
pack**, so neither sees the other's output impedance. The rework also freed one VIN and
one GND terminal.

**Verification.** The subsystems downstream were brought up individually against the
per-peripheral tests below and behaved correctly.

> This is revision 1 on the wiring diagram, in the drawing's own words: *"XL6009 5.4V
> converter: input moved OFF the 6.8v Output 'VIN' block. +IN / -IN now tap the switched
> battery pair at the 6.8V buck input, so both converters run in PARALLEL off the pack
> instead of cascaded. One VIN and one GND terminal freed."*

---

## A fault in the reference design

While redrawing the sensor wiring I found that **the PIR ground drop dead-ended on the
`VIN PIR, PS2, SW` +5 V rail** instead of the ground rail. A ground return terminating on
a power rail is not a subtle error in behaviour, but it is very easy to miss on a dense
pictogram because the two rails run parallel and the crossing looks like every other
crossing.

I corrected it to land on the GND rail, and then dealt with the reason it was missable:
**the four remaining +5 V / GND crossings were redrawn as explicit over-under**, with a
white break showing the wire passing in front. On the corrected drawing a crossing can no
longer be mistaken for a junction.

That is revision 2 on the diagram.

---

## Component substitutions

| Original | Sourced instead | Consequence |
|---|---|---|
| NRF24L01 plus custom remote | **Yahboom 2.4 G PS2 receiver** | Wired to the PS2-COM header: MISO-DAT-D7, MOSI-CMD-D8, CS-SEL-D9, SCK-CLK-D10, ACK n/c, **VCC on 3.3 V**, GND2. The D13 (SCK) drop and the NRF 22 uF cap are deleted. D9/D10 are shared with the NRF footprint, **so that footprint must stay unpopulated.** |
| DFPlayer Mini | **DFPlayer PRO (DFR0768)**, 3.3 to 5 V | Speaker moved to R+ / R- (right channel). VIN / GND / RX / TX unchanged; left channel, DAC and button pins unused. |

The PS2 substitution is also why the firmware is built on the older PS2 code path rather
than upstream's newer nRF path. See the firmware note in the
[main README](../README.md#firmware).

One caveat carried on the drawing itself: the artwork for the PS2 receiver and the
DFR0768 is **drawn, not photographic**, so the vendor pinout should be checked before
wiring.

---

## Bring-up strategy

The decision that mattered most was **not** to assemble everything, flash the full
firmware, and start guessing.

The inherited firmware is large, it drives many peripherals, and a fault could be in my
soldering, my wiring, my power rail, or somebody else's code. Debugging all of those
simultaneously is how you lose a week.

So I wrote [`Nova-SM3/Code/Tests/`](../Nova-SM3/Code/Tests/), eight standalone sketches,
one per peripheral, each of which deliberately includes **none** of the main firmware:

> *"None of them include `NovaServos.h`, `AsyncServo.h`, or any other main-sketch file,
> so a pass here proves wiring and hardware, not the main firmware."*

| Sketch | Board | Under test | Interface |
|---|---|---|---|
| `Test_PWM_Servos` | Teensy 4.0 | PCA9685 + 12 servos | I2C @ 0x40, OE pin 3 |
| `Test_PS2` | Teensy 4.0 | PS2 receiver | DAT 7, CMD 8, SEL 9, CLK 10 |
| `Test_MPU6050` | Teensy 4.0 | IMU | `Wire1`, SDA2 17 / SCL2 16, addr 0x68 |
| `Test_PIR` | Teensy 4.0 | 3x PIR | front 4, left 5, right 6 |
| `Test_MP3` | Teensy 4.0 | DFPlayer | RX 0, TX 1, volume pot 20 |
| `Test_Ultrasonic` | Arduino Nano | 2x HC-SR04 | L 7/6, R 5/4 |
| `Test_OLED` | Arduino Nano | SSD1331 96x64 | SPI 13/11/10/9/8 |
| `Test_NeoPixel` | Arduino Nano | 4x WS2812 | pin 2, brightness pot A3 |

Each carries an explicit **pass criterion**, so "it did something" is not mistaken for
"it works". This is also why the pin map above is trustworthy: it is the map the hardware
was actually verified against.

---

## Hazards worth knowing

These are in the source as warnings because they were live problems, not hypotheticals.

**The Teensy 4.0 is not 5 V tolerant.** From
[`Test_PS2.ino`](../Nova-SM3/Code/Tests/Test_PS2/Test_PS2.ino): *"The PS2 receiver is a
3.3V part. The Teensy 4.0 is ALSO 3.3V and is NOT 5V tolerant, do not feed the receiver
5V and then wire DAT back to the Teensy."* The failure mode is nasty because the receiver
*works* on 5 V. It just quietly destroys the microcontroller through the data line. The
wiring diagram accordingly puts the PS2 receiver's VCC on the 3.3 V rail.

**`OE_PIN` is active LOW, and boot order matters.** Driving OE low *enables* the servos.
It is held high through boot until the PS2 link is up, because the PWM driver interferes
with the receiver; without that sequencing the robot lurches on power-up. Servo output
arms roughly a second after boot.

**Current sense and battery sense share one pin.** `AMP_PIN` and `BATT_MONITOR` are both
**A1**. The robot physically cannot measure servo current and battery voltage at the same
time. Enabling both `amp_active` and `batt_active` without rewiring gives nonsense from
one of them. This is inherited from the original design and is an architectural
limitation rather than a bug.

**Debug flags block an untethered boot.** On a Teensy, printing to a serial port with
nothing attached stalls the board. Any debug flag left set in `NovaConfig.h` means the
robot appears completely dead when you unplug it from the laptop.

---

## Assembly

![Electronics bay detail](../hardware/media/electronics-bay-detail.jpg)

*The electronics bay, close up. Left of centre: the step-down converter, with toroidal
inductor, electrolytic bank, two finned heatsinks and screw terminals marked `IN` and
`OUT/BATT`. Right of centre: the Nova SM3 controller PCB, hand-populated with JST servo
headers, an Arduino Nano, an MPU6050 breakout, a DIP switch and screw terminals. The
masking-tape labels reading `+VCC / SDA / SCL / OE / GND` are mine, written during
bring-up so the I2C and output-enable lines could be traced against the diagram.*

The PCB is the project's own board, not an off-the-shelf module. I populated and soldered
it, then fitted it and the converter into the printed chassis. Full assembled context:
[`assembled-electronics-integration.jpg`](../hardware/media/assembled-electronics-integration.jpg).

---

## Honest gaps

- **No measurements recorded.** No current draw figures, rail voltages under load, or
  scope captures. The cascaded-converter diagnosis is supported by the corrected drawing
  and by the reasoning above, but I did not log the readings that led me there, and I am
  not going to reconstruct numbers after the fact.
- **No continuity/resistance log.** The tracing happened; the results were not written
  down.
- **No PCB photographs** beyond the integration shots.
- **No component list** for the substituted parts beyond what the diagram names, and
  still no record of which replacement servos were used.
