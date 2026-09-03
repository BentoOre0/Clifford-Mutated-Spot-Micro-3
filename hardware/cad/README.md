# CAD files

**Attribution is the point of this directory's structure.** Chris Locke's geometry lives
in `original/` and is never modified in place; my geometry lives in `modified/`.

← [back to the main README](../../README.md) ·
→ [the redesign explained](../../docs/mechanical-redesign.md)

---

## `original/` (Chris Locke's, unmodified)

Reference geometry from the upstream
[Nova SM3](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3) project,
byte-for-byte as published. Present so the before/after comparisons can be reproduced.

| File | Triangles | Envelope (mm) | Volume |
|---|---|---|---|
| `SM3_Frame_LeftCoax.stl` | 3,464 | 37.59 x 46.13 x 57.50 | 23.88 cm3 |
| `SM3_Frame_LeftFemur.stl` | 22,192 | 138.54 x 51.09 x 34.66 | 66.02 cm3 |
| `SM3_Cover_LeftFemur.stl` | 12,920 | 156.79 x 55.47 x 40.45 | 33.26 cm3 |
| `SM3_Frame_LeftTibia.stl` | 4,332 | 129.35 x 100.09 x 38.48 | 30.27 cm3 |

The frame and cover are the **two separate printed parts** that make up one original
femur. Both are here because my modified femur merges them, and the comparison needs
both halves to be honest.

Only parts with a modified counterpart are mirrored here. The complete original part set,
chassis, covers, sensor mounts and stand, is in the upstream project and is not
duplicated.

## `modified/` (mine)

Redesigned around replacement servos. Both the STL and the **native SolidWorks part** are
included, so the geometry is re-editable rather than a dead mesh.

| File | Contents |
|---|---|
| `SM3_Frame_Coxa_MOD.stl` / `.SLDPRT` | Hip bracket. 3,406 tris, 37.45 x 49.77 x 57.97 mm, 26.21 cm3 |
| `SM3_Frame_FemurTibia_MOD.stl` / `.SLDPRT` | **Two bodies on one print plate**, see below |
| `solidworks/` | Screenshots of the models and feature trees |

`SM3_Frame_FemurTibia_MOD.stl` splits by connected geometry into:

| Body | Triangles | Envelope (mm) | Volume | Is |
|---|---|---|---|---|
| 0 | 5,608 | 99.64 x 37.52 x 38.52 | 66.15 cm3 | lower leg segment |
| 1 | 5,370 | 147.13 x 68.49 x 35.66 | 90.73 cm3 | femur, frame + cover merged |

Body 1 is the change worth noting: the original femur is two printed parts that bolt
together (56.98 + 33.26 = 90.24 cm3), and this is those two merged into one solid
(90.73 cm3, a 0.5% difference). See
[the redesign write-up](../../docs/mechanical-redesign.md#femur-two-parts-merged-into-one)
for why.

The SolidWorks trees begin with `Imported1`, the original STL imported as a base body,
followed by my own sketches and features. That is the provenance: modification on top of
the original, not a redraw and not a mesh edit.

### A note on naming

Upstream spells the hip part "Coax"; the anatomical term, and the one the firmware uses
in its servo names (`RFC` = right front **coxa**), is "coxa". My files use `Coxa`. They
are the same joint.

## `comparisons/` (generated figures)

Rendered from the STL files in `original/` and `modified/` at matched camera angles, with
measured dimensions. Regenerable from the geometry alone; nothing here is drawn by hand.

| Figure | Shows |
|---|---|
| `coxa-comparison.png` | Hip bracket, original vs modified |
| `coxa-servo-pocket.png` | Down the joint axis: same horn pattern, different pocket |
| `femur-comparison.png` | Original frame + cover, against my merged single part |
| `tibia-comparison.png` | Lower leg, original vs modified |

---

## Licence

`original/` is Chris Locke's work under the upstream project's terms. `modified/` is
derived from it and carries the same obligations. Neither is public domain. If you use
either, credit [novaspotmicro.com](https://novaspotmicro.com).
