# Rooster Scene Format (`.rscn`) Reference

All scene files must start with:

```text
ROOSTERSCENEV1
```

## Global render/camera directives

```text
viewport <width> <height>
fov <degrees>
camera <px> <py> <pz>  <lookx> <looky> <lookz>  <upx> <upy> <upz>  <near> <far>
background <R> <G> <B>
max_depth <integer>
```

- `R G B` values are in `[0,255]`.
- Camera/light/object vectors are in world space.

## Materials

```text
mat <R> <G> <B>
```

- Appends a new material in order (`0, 1, 2, ...`).
- `plane` and `obj` can reference a material by index.
- `sphere` and `triangle` create/reuse inline materials from their color.

## Lights

```text
dir_light <dx> <dy> <dz> <R> <G> <B> <intensity>
point_light <px> <py> <pz> <R> <G> <B> <intensity>
```

## Geometry

```text
sphere <x> <y> <z> <radius> <R> <G> <B>
triangle <x1> <y1> <z1>  <x2> <y2> <z2>  <x3> <y3> <z3>  <R> <G> <B>
plane <px> <py> <pz>  <nx> <ny> <nz>  <material_id>
obj <path.obj> [ox oy oz] [material_id]
```

- `obj` path can be absolute or relative to the `.rscn` file directory.
- `ox oy oz` and `material_id` are optional.

## Comments

Lines starting with `#` are ignored.

