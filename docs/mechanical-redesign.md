# Mechanical redesign

How the Nova SM3 leg parts were adapted for servos the original design was never
drawn around.

← [back to the main README](../README.md)

---

## The problem

The Nova SM3 bill of materials specifies particular servos. I could not practically
source them, so I bought what was available.

The replacement servos have a different case size and a different mounting hole pattern.
Every part that touches a servo is therefore wrong: the pocket that captures the case,
the bosses the screws land in, and the clearance around the whole assembly.

What is *not* necessarily wrong is the **output horn**. If the replacement servo's horn
spline and its bolt circle match, then the point where the servo's rotation enters the
mechanism can stay exactly where the original put it.

That distinction is the whole design.

---

## The rule

> **Preserve the horn interface exactly. Rebuild everything around it.**

The horn interface is where the joint axis lives. Move it by a few millimetres and every
link length in the leg changes, which means the gait tuning in
[`Gaits.ino`](../Nova-SM3/Code/Nova-SM3_teensy-v6.0/Gaits.ino) — hundreds of hand-tuned
multipliers arrived at on real hardware, with no inverse kinematics to fall back on —
becomes meaningless.

Everything else is just structure. Structure can be rebuilt.

---

## How the parts were actually modified

Not redrawn from scratch, and not edited as meshes either. The workflow was:

1. **Import the original STL into SolidWorks as a base solid body.** This appears in the
   feature tree as `Imported1`.
2. **Cut away** the geometry that assumed the original servo — pockets, bosses, brackets.
3. **Add native parametric features** — sketches, boss-extrudes, cut-extrudes — for the
   replacement servo.
4. **Mirror** where a left/right pair is needed, and lay parts out on a print plate.

You can see this directly in the feature trees:

<table>
<tr>
<td width="50%"><img src="../hardware/cad/modified/solidworks/coax-model.png" alt="Modified coax feature tree in SolidWorks"></td>
<td width="50%"><img src="../hardware/cad/modified/solidworks/femur-feature-tree.png" alt="Modified femur feature tree in SolidWorks"></td>
</tr>
<tr>
<td colspan="2"><em>Left: <code>coaxmodified_fixed</code>. The tree starts with <code>Imported1</code> — Chris Locke's STL — and then carries my own <code>Sketch2/4/7</code>, <code>Boss-Extrude1/4/5/11–15</code> and <code>Cut-Extrude1</code> on top of it. Right: the femur part, with roughly twenty-five sketch and feature operations. This is why the modification is parametric and re-editable rather than a one-way mesh edit.</em></td>
</tr>
</table>

This matters for a practical reason as well as a provenance one: because the changes are
parametric features on top of an imported body, a different servo can be accommodated by
editing a sketch dimension rather than starting again.

---

## Part-by-part

All dimensions below are **measured from the mesh geometry** of the files in
[`../hardware/cad/`](../hardware/cad/), not estimated or recalled.

### Hip / coxa

![Coxa comparison](../hardware/cad/comparisons/coax-comparison.png)

| | Original `SM3_Frame_LeftCoax` | Modified `coax_modified` | Δ |
|---|---|---|---|
| Envelope | 37.59 × 46.13 × 57.50 mm | 37.45 × 49.77 × 57.97 mm | +3.64 mm across |
| Volume | 23.88 cm³ | 26.21 cm³ | +9.8% |
| Bodies | 1 | 1 | — |

The axis-direction envelope changes by **0.14 mm** — that is the interface being held.
The 3.64 mm growth is all in the transverse direction, which is the pocket opening up
for a wider servo case.

Structurally, the original's full-height flat back plate with an overhanging two-hole
ear becomes a stepped C-bracket: a narrow vertical web, a broad cantilevered top shelf,
and a separate lower foot with a boss.

![Coxa servo pocket](../hardware/cad/comparisons/coax-servo-pocket.png)

Down the joint axis, the five-hole horn pattern is identical between the two parts. The
pocket is not: rounded-square and concentric in the original, larger and angular with
relieved corners and offset from the horn centreline in mine.

### Femur

![Femur comparison](../hardware/cad/comparisons/femur-comparison.png)

| | Original `SM3_Frame_LeftFemur` (main body) | Modified femur | Δ |
|---|---|---|---|
| Envelope | 138.54 × 44.47 × 34.66 mm | 147.13 × 68.49 × 35.66 mm | +8.6 / +24.0 / +1.0 mm |
| Volume | 56.98 cm³ | 90.73 cm³ | +59% |

The hip-end horn hub is preserved. The knee end is rebuilt completely: the original's
slim slotted fork becomes a flat mounting pad with square posts and a boxed section that
captures the knee servo.

A **59% increase in material** is a real cost, in print time, filament and — more
importantly — in mass hanging off a joint that has to accelerate it. It is the price of
the servo substitution, and it is worth stating plainly rather than hiding.

### Lower leg

![Lower leg comparison](../hardware/cad/comparisons/tibia-comparison.png)

| | Original `SM3_Frame_LeftTibia` | Modified lower-leg segment | Δ |
|---|---|---|---|
| Envelope | 129.35 × 100.09 × 38.48 mm | 99.64 × 37.52 × 38.52 mm | see note |
| Volume | 30.27 cm³ | 66.15 cm³ | +118% |

The original tibia is a slim curved arm — knee bore at one end, light bracket at the
other. The modified segment keeps a knee pivot boss but carries a **full rectangular
servo enclosure** where the original had an open bracket, which is where the doubled
volume comes from.

The envelope numbers are not directly comparable here, because the original tibia is a
single part spanning knee to foot while the modified lower leg is split across more than
one body. The separate lower part, `legbottom2`, is the one that carries the **TPU foot
mounting** through from the original design:

![legbottom2 in SolidWorks](../hardware/cad/modified/solidworks/tibia-legbottom2.png)

*`legbottom2` — the lower tibia section, with the tapered foot end that takes the TPU
foot. The foot mounting was preserved deliberately: the TPU feet were one of the few
original components I could still use.*

---

## About `legs_v67_printplate.stl`

The modified leg file is a **print plate containing two separate solid bodies**, not a
single part. Splitting it by connected geometry gives:

| Body | Triangles | Envelope | Volume |
|---|---|---|---|
| 0 — lower leg segment | 5,608 | 99.64 × 37.52 × 38.52 mm | 66.15 cm³ |
| 1 — femur | 5,370 | 147.13 × 68.49 × 35.66 mm | 90.73 cm³ |

The comparison figures above render each body separately so that each is compared
against the correct original part. The `v67` in the filename is not decoration — it is
the revision that actually got printed.

---

## What is not documented here

Being explicit, because a portfolio that only shows the parts that went well is not
worth much:

- **No dimensioned drawings.** The comparisons are rendered geometry and measured
  envelopes, not a formal drawing package with tolerances.
- **No modified tibia STL export.** The lower tibia part (`legbottom2`) is evidenced by
  SolidWorks screenshots only; it was not exported as a standalone STL into the set I
  archived.
- **No print settings or material record.** Layer height, infill, material and
  orientation were not logged, and I am not going to reconstruct them from memory.
- **No load analysis.** The parts were sized by judgement and iteration — the `v67`
  revision number is a fair summary of the method — not by FEA or measured joint torque.
- **Which specific servos.** I have not recorded the exact model of the replacement
  servos anywhere in this repository, which in hindsight is the single most useful thing
  I could have written down.
