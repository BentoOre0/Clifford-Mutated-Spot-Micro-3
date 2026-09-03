# Electronics artifacts

← [electronics write-up](../../docs/electronics.md)

| File | What it is |
|---|---|
| `Wiring_NovaSM3_v5-2b_MOD.png` | The wiring diagram for this build, 6000 x 4712. Open it full size to read pin labels. |
| `wiring-revision-notes.png` | The revision block from that drawing, cropped for legibility. |

## About the diagram

It is a revision of Chris Locke's **v5.2b pictogram**, updated for the components I could
actually source and for one fault I found in the reference design. **The base artwork is
his; the revisions and the revision block are mine.**

It is committed as the drawing itself, at full resolution, rather than redrawn or
regenerated, so the technical content is the artifact rather than a description of it.

The five revisions are listed on the drawing. In summary:

1. **Power architecture.** The XL6009 5.4 V converter's input moved off the 6.8 V output
   block onto the switched battery pair, so both converters run **in parallel off the
   pack instead of cascaded**.
2. **Corrected wiring fault.** The PIR ground drop that dead-ended on the `VIN PIR, PS2,
   SW` +5 V rail now lands on the GND rail, and the remaining +5 V / GND crossings are
   redrawn as explicit over-under.
3. **NRF24L01 removed**, replaced by a Yahboom 2.4 G PS2 receiver on the PS2-COM header,
   VCC on 3.3 V. The NRF footprint must stay unpopulated because D9/D10 are shared.
4. **DFPlayer Mini replaced by DFPlayer PRO (DFR0768)**, speaker on the right channel.
5. A caveat that the PS2 receiver and DFR0768 artwork is drawn rather than photographic,
   so vendor pinouts should be checked before wiring.

Items 1 and 2 are explained in full, with the observation and reasoning behind them, in
[`docs/electronics.md`](../../docs/electronics.md).

Upstream's unmodified schematic and pictogram are **not** reproduced here. They are Chris
Locke's documents and are available from
[the upstream project](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3).
