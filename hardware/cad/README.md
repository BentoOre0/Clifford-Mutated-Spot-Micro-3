# CAD files

**Attribution is the point of this directory's structure.** Chris Locke's geometry lives
in `original/` and is never modified in place; my geometry lives in `modified/`.

← [back to the main README](../../README.md) ·
→ [the redesign explained](../../docs/mechanical-redesign.md)

---

## `original/` — Chris Locke's, unmodified

Reference geometry from the upstream
[Nova SM3](https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3) project,
byte-for-byte as published. Present so the before/after comparisons can be reproduced.

| File | Triangles | Envelope (mm) | Volume |
|---|---|---|---|
| `SM3_Frame_LeftCoax.stl` | 3,464 | 37.59 × 46.13 × 57.50 | 23.88 cm³ |
| `SM3_Frame_LeftFemur.stl` | 22,192 | 138.54 × 51.09 × 34.66 | 66.02 cm³ |
| `SM3_Frame_LeftTibia.stl` | 4,332 | 129.35 × 100.09 × 38.48 | 30.27 cm³ |

Only the parts with a modified counterpart are mirrored here. The complete original
part set — chassis, covers, sensor mounts, stand — is in the upstream project and is not
duplicated.

## `modified/` — mine

Redesigned around replacement servos. Both the STL and the **native SolidWorks part**
are included, so the geometry is re-editable rather than a dead mesh.

| File | Contents |
|---|---|
| `coax_modified.stl` / `.SLDPRT` | Hip bracket. 3,406 tris, 37.45 × 49.77 × 57.97 mm, 26.21 cm³ |
| `legs_v67_printplate.stl` / `.SLDPRT` | **Two bodies on one print plate** — see below |
| `solidworks/` | Screenshots of the SolidWorks models and feature trees |

`legs_v67_printplate.stl` splits by connected geometry into:

| Body | Triangles | Envelope (mm) | Volume | Is |
|---|---|---|---|---|
| 0 | 5,608 | 99.64 × 37.52 × 38.52 | 66.15 cm³ | lower leg segment |
| 1 | 5,370 | 147.13 × 68.49 × 35.66 | 90.73 cm³ | femur |

The SolidWorks trees begin with `Imported1` — the original STL, imported as a base body
— followed by my own sketches and features. That is the provenance: modification on top
of the original, not a redraw and not a mesh edit.

## `comparisons/` — generated figures

Rendered from the STL files in `original/` and `modified/` at matched camera angles,
with measured dimensions. Regenerable from the geometry alone; nothing here is drawn by
hand.

| Figure | Shows |
|---|---|
| `coax-comparison.png` | Hip bracket, original vs modified |
| `coax-servo-pocket.png` | Down the joint axis — same horn pattern, different pocket |
| `femur-comparison.png` | Femur, original vs modified |
| `tibia-comparison.png` | Lower leg, original vs modified |

---

## Licence

`original/` is Chris Locke's work under the upstream project's terms. `modified/` is
derived from it and carries the same obligations. Neither is public domain — if you use
either, credit [novaspotmicro.com](https://novaspotmicro.com).
