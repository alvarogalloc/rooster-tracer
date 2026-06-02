# Rooster Scene Format (`.rscn`) Reference

All scene files must begin with the magic token on the very first line:

```
ROOSTERSCENEV1
```

Lines starting with `#` are comments and are ignored.
Directives are **order-sensitive**: a `mat` must appear before any command that
references it by index; `mtllib` materials inside an OBJ are resolved at load
time and do not depend on order in the `.rscn`.

---

## Global / Camera Directives

```
viewport <width> <height>
fov      <degrees>
camera   <px> <py> <pz>   <lx> <ly> <lz>   <ux> <uy> <uz>   <near> <far>
background <R> <G> <B>
hdr path/to/image.hdr
max_depth  <integer>
```

| Directive    | Description                                                  |
| ------------ | ------------------------------------------------------------ |
| `viewport`   | Output image resolution in pixels.                           |
| `fov`        | Vertical field of view in degrees.                           |
| `camera`     | Position, look-at point, up vector, near/far clip distances. |
| `background` | Sky colour when no geometry is hit, in 0–255 RGB.            |
| `max_depth`  | Maximum ray bounce depth (1 = direct lighting only).         |

All positions and directions are in **Y-up, right-handed** world space
(matching Blender's OBJ export with `-Z` forward / `Y` up).

---

## Materials (`mat`)

```
mat r g b [reflectivity] [transparency] [ior]
```

- Appends one Phong material with the given diffuse colour (0–255 per channel).
- Materials are numbered sequentially starting from `0`.
- **`mat` is intended for hardcoded / hand-written scenes.** When loading OBJ
  files exported from Blender (or any tool that writes an accompanying `.mtl`),
  the real per-face materials come from the `.mtl` — `mat` just provides the
  **fallback** used for faces that have no `usemtl` directive.
- Best practice when using the `blend_to_rscn` exporter: declare a single
  fallback `mat 180 180 190` and let the `.mtl` handle everything else.

---

## Lights

```
dir_light   <dx> <dy> <dz>  <R> <G> <B>  <intensity>
point_light <px> <py> <pz>  <R> <G> <B>  <intensity>
```

| Parameter   | Description                                                               |
| ----------- | ------------------------------------------------------------------------- |
| `dx dy dz`  | Direction the light shines **toward** (normalised automatically).         |
| `px py pz`  | World-space position of the point light.                                  |
| `R G B`     | Light colour in 0–255.                                                    |
| `intensity` | Scalar multiplier. Point-light radiance falls off as `intensity / dist²`. |

> **`blend_to_rscn` note:** Blender light energy is divided by 100 to produce
> the `intensity` value. A Blender point light at 1000 W becomes `intensity 10`.

---

## Geometry

### Sphere

```
sphere <cx> <cy> <cz>  <radius>  <R> <G> <B>
```

Creates an inline Phong material from the colour and reuses it if an identical
material already exists.

### Triangle

```
triangle <x1> <y1> <z1>   <x2> <y2> <z2>   <x3> <y3> <z3>   <R> <G> <B>
```

Same inline-material behaviour as `sphere`.

### Plane (infinite)

```
plane <px> <py> <pz>   <nx> <ny> <nz>   <material_id>
```

References a material by its `mat` index.

### OBJ Mesh

```
obj <path.obj>  [ox oy oz]  [material_id]
```

| Argument      | Default | Description                                                                   |
| ------------- | ------- | ----------------------------------------------------------------------------- |
| `path.obj`    | —       | Path to the OBJ file. Relative paths are resolved from the `.rscn` directory. |
| `ox oy oz`    | `0 0 0` | World-space translation added to every vertex.                                |
| `material_id` | `0`     | Fallback `mat` index used for faces that carry no `usemtl`.                   |

#### MTL material pipeline

When the OBJ file contains a `mtllib` directive, the referenced `.mtl` is
parsed automatically. The following MTL fields are supported:

| MTL field                                        | Effect                                                                                                                                                                                                          |
| ------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `newmtl <name>`                                  | Begins a new named material.                                                                                                                                                                                    |
| `Kd r g b`                                       | Diffuse colour (linear 0–1 floats).                                                                                                                                                                             |
| `Ks r g b`                                       | Specular colour.                                                                                                                                                                                                |
| `Ka r g b`                                       | Ambient tint. **Blender always exports `Ka 1 1 1`**, so when all three channels are ≥ 0.99 the value is ignored and ambient is derived from `Kd × 0.1` instead. A non-white `Ka` is honoured but scaled by 0.1. |
| `Ns value`                                       | Phong shininess exponent (0–1000).                                                                                                                                                                              |
| `Ke r g b`                                       | Emissive colour (parsed, reserved for future use).                                                                                                                                                              |
| `d value`                                        | Dissolve/opacity (parsed, reserved).                                                                                                                                                                            |
| `Tr value`                                       | Transparency; stored as `d = 1 − Tr`.                                                                                                                                                                           |
| `illum model`                                    | Illumination model integer (parsed, reserved).                                                                                                                                                                  |
| `Ni value`                                       | Index of refraction (parsed, reserved).                                                                                                                                                                         |
| `map_Kd <file>`                                  | Diffuse texture; path relative to the `.mtl` file. Multiplied with `Kd`.                                                                                                                                        |
| `map_Ks <file>`                                  | Specular texture (parsed, reserved).                                                                                                                                                                            |
| `map_Bump`, `map_bump`, `bump`, `map_Kn`, `norm` | Normal/bump map; optional `-bm <scale>` prefix is consumed.                                                                                                                                                     |

Each `usemtl <name>` in the OBJ face section switches the active material.
MTL materials are appended to the scene material list after any `mat` entries
defined in the `.rscn`, so their IDs are stable across a single load.

---

## Blender → `.rscn` Workflow (`blend_to_rscn`)

```
python tools/blend_to_rscn.py  <scene.blend>  <assets_dir>  <output.rscn>  [--blender <exe>]
```

1. Launches Blender in background mode.
2. Exports each mesh object as an individual `.obj` + `.mtl` into `assets_dir`.
   Transforms are baked into world space; the OBJ vertex colours are **not**
   written — all colour information lives in the `.mtl`.
3. Reads camera, lights, and world background from the Blender scene.
4. Writes a `.rscn` containing:
   - One `mat 180 180 190` fallback (for OBJ faces with no `usemtl`).
   - Lights converted from Blender coordinates (Z-up → Y-up).
   - One `obj` line per mesh, all with fallback `material_id 0`.

Material colours are **not** duplicated into the `.rscn`; they come exclusively
from the exported `.mtl` files via `usemtl`.
