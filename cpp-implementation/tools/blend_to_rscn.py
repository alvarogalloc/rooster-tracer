#!/usr/bin/env python3
"""
blend_to_rscn.py – Convert a Blender .blend scene to a Rooster .rscn scene file.

Usage:
    python blend_to_rscn.py <scene.blend> <assets_dir> <output.rscn> [--blender <blender_exe>]

This is a standalone script that:
    1. Launches Blender in background mode with an embedded export script
    2. The embedded script exports camera, lights, and mesh objects to individual OBJ files
    3. Reads the manifest and assembles a .rscn file

Requirements:
    - Blender 3.x / 4.x / 5.x accessible on PATH (or via --blender argument).
    - Python 3.10+ (standard library only).
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path


# =============================================================================
# Embedded Blender export script (runs inside Blender's Python interpreter)
# =============================================================================

BLENDER_EXPORT_SCRIPT = r'''
import bpy
import os
import sys
import json
import math
import re
import shutil

# ---- Args passed via environment ----
manifest_path = os.environ["ROOSTER_MANIFEST"]
assets_dir    = os.environ["ROOSTER_ASSETS_DIR"]
os.makedirs(assets_dir, exist_ok=True)

scene     = bpy.context.scene
depsgraph = bpy.context.evaluated_depsgraph_get()

manifest = {
    "viewport":   [scene.render.resolution_x, scene.render.resolution_y],
    "camera":     None,
    "background": [12, 12, 18],
    "lights":     [],
    "objects":    [],
}

# ---------------------------------------------------------------------------
# Camera
# ---------------------------------------------------------------------------
cam_obj = scene.camera
if cam_obj:
    import mathutils
    cam = cam_obj.data
    loc = cam_obj.matrix_world.to_translation()

    fwd     = cam_obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 0, -1))
    up      = cam_obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 1, 0))
    look_at = loc + fwd

    near = cam.clip_start
    far  = cam.clip_end

    def b2r(v):
        return [v.x, v.z, -v.y]

    aspect = scene.render.resolution_x / max(scene.render.resolution_y, 1)
    sensor_fit = cam.sensor_fit
    if sensor_fit == 'VERTICAL':
        fov_v_rad = cam.angle
    else:
        h_fov = cam.angle
        fov_v_rad = 2.0 * math.atan(math.tan(h_fov / 2.0) / aspect)
    fov_deg = math.degrees(fov_v_rad)

    manifest["camera"] = {
        "pos":     b2r(loc),
        "look_at": b2r(look_at),
        "up":      b2r(up),
        "fov":     fov_deg,
        "near":    near,
        "far":     far,
    }

# ---------------------------------------------------------------------------
# World / background
# ---------------------------------------------------------------------------
world = scene.world
if world and world.use_nodes:
    for node in world.node_tree.nodes:
        if node.type == "BACKGROUND":
            col = node.inputs["Color"].default_value
            strength = node.inputs.get("Strength", None)
            s = strength.default_value if strength else 1.0
            r = min(255, int(col[0] * 255 * s))
            g = min(255, int(col[1] * 255 * s))
            b = min(255, int(col[2] * 255 * s))
            manifest["background"] = [r, g, b]
            break

# ---------------------------------------------------------------------------
# Lights
# ---------------------------------------------------------------------------
def b2r_plain(x, y, z):
    return [x, z, -y]

for obj in scene.objects:
    if obj.type != "LIGHT":
        continue
    light = obj.data
    loc = obj.matrix_world.to_translation()
    col = light.color
    r = min(255, int(col[0] * 255))
    g = min(255, int(col[1] * 255))
    b = min(255, int(col[2] * 255))
    intensity = light.energy / 100.0

    if light.type == "SUN":
        import mathutils
        fwd = obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 0, -1))
        dx, dy, dz = b2r_plain(fwd.x, fwd.y, fwd.z)
        manifest["lights"].append({
            "type": "dir_light",
            "dir": [dx, dy, dz],
            "color": [r, g, b],
            "intensity": max(0.1, intensity),
        })
    elif light.type in ("POINT", "AREA", "SPOT"):
        px, py, pz = b2r_plain(loc.x, loc.y, loc.z)
        manifest["lights"].append({
            "type": "point_light",
            "pos": [px, py, pz],
            "color": [r, g, b],
            "intensity": max(0.1, intensity),
        })

if not manifest["lights"]:
    manifest["lights"].append({
        "type": "dir_light",
        "dir": [-0.7, -1.0, -0.2],
        "color": [255, 255, 255],
        "intensity": 0.9,
    })
    manifest["lights"].append({
        "type": "point_light",
        "pos": [2.0, 3.0, 2.0],
        "color": [255, 230, 200],
        "intensity": 3.0,
    })

# ---------------------------------------------------------------------------
# Mesh objects
# ---------------------------------------------------------------------------
mesh_objects = [o for o in scene.objects if o.type == "MESH"]

_MAP_KW_RE = re.compile(
    r"^(\s*(?:map_Kd|map_Ks|map_Ka|map_Bump|map_bump|bump|map_Kn|norm|map_d)\s+)",
    re.IGNORECASE,
)

# First pass: collect and copy all textures
tex_remap = {}
for obj in mesh_objects:
    for slot in obj.material_slots:
        mat = slot.material
        if not mat or not mat.use_nodes:
            continue
        for node in mat.node_tree.nodes:
            if node.type != "TEX_IMAGE" or not node.image:
                continue
            img = node.image

            if img.packed_file:
                raw_name = bpy.path.basename(img.filepath) if img.filepath else img.name
                if not raw_name or raw_name == "<none>":
                    raw_name = img.name
                if not any(raw_name.lower().endswith(ext)
                           for ext in (".png", ".jpg", ".jpeg", ".tga", ".bmp", ".exr", ".hdr")):
                    raw_name += ".png"
                save_path = os.path.join(assets_dir, raw_name)
                if not os.path.exists(save_path):
                    try:
                        old_fp = img.filepath_raw
                        img.filepath_raw = save_path
                        img.save()
                        img.filepath_raw = old_fp
                        print(f"[blend_to_rscn] Unpacked image: {raw_name}")
                    except Exception as exc:
                        print(f"[blend_to_rscn] WARNING: could not unpack {img.name}: {exc}")
                tex_remap[img.name] = raw_name
                continue

            src = bpy.path.abspath(img.filepath)
            if not os.path.isfile(src):
                print(f"[blend_to_rscn] WARNING: texture not found: {src}")
                continue
            raw_name = os.path.basename(src)
            dst = os.path.join(assets_dir, raw_name)
            if os.path.abspath(src) != os.path.abspath(dst):
                try:
                    shutil.copy2(src, dst)
                    print(f"[blend_to_rscn] Copied texture: {raw_name}")
                except Exception as exc:
                    print(f"[blend_to_rscn] WARNING: could not copy {src}: {exc}")
            tex_remap[src] = raw_name

# Second pass: export each mesh individually
for obj in mesh_objects:
    eval_obj = obj.evaluated_get(depsgraph)
    mesh_data = eval_obj.to_mesh()
    if not mesh_data or not mesh_data.vertices:
        eval_obj.to_mesh_clear()
        continue

    safe_name = bpy.path.clean_name(obj.name).replace(" ", "_")
    obj_filename = safe_name + ".obj"
    obj_path = os.path.join(assets_dir, obj_filename)

    # Select only this object
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    export_ok = False
    try:
        bpy.ops.wm.obj_export(
            filepath=obj_path,
            export_selected_objects=True,
            apply_modifiers=True,
            forward_axis="NEGATIVE_Z",
            up_axis="Y",
            export_materials=True,
            export_triangulated_mesh=True,
            global_scale=1.0,
        )
        export_ok = True
    except Exception as e4:
        try:
            bpy.ops.export_scene.obj(
                filepath=obj_path,
                use_selection=True,
                use_mesh_modifiers=True,
                axis_forward="-Z",
                axis_up="Y",
                use_materials=True,
                use_triangles=True,
                global_scale=1.0,
            )
            export_ok = True
        except Exception as e3:
            print(f"[blend_to_rscn] OBJ export failed for {obj.name}: {e4} / {e3}")

    eval_obj.to_mesh_clear()

    if not export_ok:
        continue

    # Patch texture paths in the .mtl
    mtl_path = obj_path[:-4] + ".mtl"
    if os.path.isfile(mtl_path) and tex_remap:
        with open(mtl_path, "r", encoding="utf-8", errors="replace") as fh:
            mtl_lines = fh.readlines()
        patched = []
        for line in mtl_lines:
            m = _MAP_KW_RE.match(line)
            if m:
                rest = line[len(m.group(0)):].strip()
                tokens = rest.split()
                orig = tokens[-1] if tokens else ""
                bare = os.path.basename(orig)
                for old_key, new_name in tex_remap.items():
                    if os.path.basename(old_key) == bare or old_key == orig:
                        bare = new_name
                        break
                flags = " ".join(tokens[:-1])
                line = m.group(0) + (flags + " " if flags else "") + bare + "\n"
            patched.append(line)
        with open(mtl_path, "w", encoding="utf-8") as fh:
            fh.writelines(patched)
        print(f"[blend_to_rscn] Patched texture paths in: {os.path.basename(mtl_path)}")

    manifest["objects"].append({
        "name": obj.name,
        "obj_file": obj_filename,
        "offset": [0.0, 0.0, 0.0],
    })

# ---------------------------------------------------------------------------
# Write manifest
# ---------------------------------------------------------------------------
with open(manifest_path, "w") as f:
    json.dump(manifest, f, indent=2)

print(f"[blend_to_rscn] Manifest written to {manifest_path}")
print(f"[blend_to_rscn] Exported {len(manifest['objects'])} mesh object(s).")
'''


# =============================================================================
# Host-side helpers
# =============================================================================

def find_blender(hint: str | None) -> str:
    """Return path to the blender executable."""
    if hint:
        return hint
    for name in ("blender", "blender.exe"):
        found = shutil.which(name)
        if found:
            return found
    # Common install locations on Windows
    for candidate in [
        r"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 5.0\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.4\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.1\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.0\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 3.6\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender\blender.exe",
    ]:
        if os.path.isfile(candidate):
            return candidate
    raise FileNotFoundError(
        "Could not find Blender. Install it or pass --blender /path/to/blender."
    )


def run_blender_export(blend_file: Path, assets_dir: Path, blender_exe: str) -> dict:
    """Run Blender in background mode to export the scene; return the manifest dict."""
    with tempfile.TemporaryDirectory(prefix="rooster_blend_") as tmpdir:
        manifest_path = os.path.join(tmpdir, "manifest.json")

        # Write the embedded script to a temp file
        script_path = os.path.join(tmpdir, "blender_export_script.py")
        with open(script_path, "w", encoding="utf-8") as f:
            f.write(BLENDER_EXPORT_SCRIPT)

        env = os.environ.copy()
        env["ROOSTER_MANIFEST"] = manifest_path
        env["ROOSTER_ASSETS_DIR"] = str(assets_dir)

        cmd = [
            blender_exe,
            "--background",
            str(blend_file),
            "--python", script_path,
        ]

        print(f"[blend_to_rscn] Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, env=env, capture_output=False)
        if result.returncode != 0:
            print(f"[blend_to_rscn] Blender exited with code {result.returncode}.",
                  file=sys.stderr)
            sys.exit(1)

        if not os.path.isfile(manifest_path):
            print("[blend_to_rscn] ERROR: manifest not written by Blender script.",
                  file=sys.stderr)
            sys.exit(1)

        with open(manifest_path, "r", encoding="utf-8") as f:
            manifest = json.load(f)

    return manifest


# =============================================================================
# .rscn assembly
# =============================================================================

def vec3_str(v: list[float]) -> str:
    return " ".join(f"{x:.4f}" for x in v)


def build_rscn(manifest: dict, assets_dir: Path, output_rscn: Path) -> None:
    """Assemble the .rscn file from the exported manifest."""
    lines: list[str] = ["ROOSTERSCENEV1"]

    # -- Viewport
    vp = manifest.get("viewport", [1280, 720])
    lines.append(f"viewport {vp[0]} {vp[1]}")

    # -- Camera
    cam = manifest.get("camera")
    if cam:
        fov = cam.get("fov", 60.0)
        lines.append(f"fov {fov:.2f}")
        pos = cam["pos"]
        look_at = cam["look_at"]
        up = cam.get("up", [0, 1, 0])
        near = cam.get("near", 0.1)
        far = cam.get("far", 100.0)
        lines.append(
            f"camera {vec3_str(pos)}   {vec3_str(look_at)}   {vec3_str(up)}   {near:.3f} {far:.1f}"
        )
    else:
        lines.append("fov 60")
        lines.append("camera 0 3 6   0 0 0   0 1 0   0.1 100")

    # -- Background
    bg = manifest.get("background", [12, 12, 18])
    lines.append(f"background {bg[0]} {bg[1]} {bg[2]}")

    # -- Render depth
    lines.append("max_depth 2")
    lines.append("")

    # -- Default fallback material
    lines.append("# default fallback material – overridden per-face by .mtl usemtl")
    lines.append("mat 180 180 190")
    lines.append("")

    # -- Lights
    for light in manifest.get("lights", []):
        if light["type"] == "dir_light":
            d = light["dir"]
            c = light["color"]
            iv = light["intensity"]
            lines.append(
                f"dir_light {vec3_str(d)} {c[0]} {c[1]} {c[2]} {iv:.2f}"
            )
        elif light["type"] == "point_light":
            p = light["pos"]
            c = light["color"]
            iv = light["intensity"]
            lines.append(
                f"point_light {vec3_str(p)} {c[0]} {c[1]} {c[2]} {iv:.2f}"
            )

    lines.append("")

    # -- Objects
    rscn_dir = output_rscn.parent.resolve()
    assets_dir_resolved = assets_dir.resolve()
    try:
        rel_assets = os.path.relpath(assets_dir_resolved, rscn_dir)
    except ValueError:
        # Different drives on Windows
        rel_assets = str(assets_dir_resolved)

    for obj in manifest.get("objects", []):
        obj_file = obj["obj_file"]
        offset = obj.get("offset", [0.0, 0.0, 0.0])
        rel_obj = os.path.join(rel_assets, obj_file).replace("\\", "/")
        ox, oy, oz = offset
        lines.append(f"obj {rel_obj} {ox:.4f} {oy:.4f} {oz:.4f} 0")

    content = "\n".join(lines) + "\n"
    output_rscn.parent.mkdir(parents=True, exist_ok=True)
    output_rscn.write_text(content, encoding="utf-8")
    print(f"[blend_to_rscn] Written: {output_rscn}")
    print(f"[blend_to_rscn] Scene contains {len(manifest.get('objects', []))} mesh(es) "
          f"and {len(manifest.get('lights', []))} light(s).")


# =============================================================================
# Main
# =============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert a Blender .blend scene to a Rooster Raytracer .rscn file.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Example:
                python blend_to_rscn.py \\
                    blenderscenes/samplecube.blend \\
                    assets/from_blender \\
                    scenes/samplecube.rscn
        """),
    )
    parser.add_argument("blend_file", type=Path, help="Path to the .blend file")
    parser.add_argument("assets_dir", type=Path, help="Directory for exported OBJ/texture/material files")
    parser.add_argument("output_rscn", type=Path, help="Path for the generated .rscn file")
    parser.add_argument(
        "--blender", metavar="EXE", default=None,
        help="Path to the Blender executable (auto-detected if not given)"
    )
    args = parser.parse_args()

    blend_file = args.blend_file.resolve()
    assets_dir = args.assets_dir.resolve()
    output_rscn = args.output_rscn.resolve()

    if not blend_file.exists():
        print(f"[blend_to_rscn] ERROR: blend file not found: {blend_file}", file=sys.stderr)
        sys.exit(1)

    blender_exe = find_blender(args.blender)
    print(f"[blend_to_rscn] Using Blender: {blender_exe}")

    assets_dir.mkdir(parents=True, exist_ok=True)

    # Step 1: Export from Blender
    print("[blend_to_rscn] Step 1/2 – Exporting from Blender …")
    manifest = run_blender_export(blend_file, assets_dir, blender_exe)

    # Step 2: Assemble .rscn
    print("[blend_to_rscn] Step 2/2 – Assembling .rscn …")
    build_rscn(manifest, assets_dir, output_rscn)

    print()
    print("Done! To render:")
    print(f"    ./build/<platform>/release/cpp-raytracer {output_rscn}")


if __name__ == "__main__":
    main()
