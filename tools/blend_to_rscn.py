#!/usr/bin/env python3
"""
blend_to_rscn.py – Convert a Blender .blend scene to a Rooster .rscn file.

Usage:
    python blend_to_rscn.py <scene.blend> <assets_dir> <output.rscn> [--blender <exe>]

Requirements:
    - Blender 3.x / 4.x / 5.x accessible on PATH (or via --blender).
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
# Embedded Blender export script
# Runs inside Blender's Python interpreter via --python.
# Must be completely self-contained.
#
# Key design constraint: Blender background mode has no 3D viewport, so
# bpy.ops that poll() for an OBJECT-mode area (select_all, select_set, etc.)
# will raise RuntimeError. We avoid all such operators.
#
# Per-object OBJ export strategy:
#   - Temporarily hide every mesh except the target via obj.hide_viewport
#   - Call bpy.ops.wm.obj_export with export_selected_objects=False
#     → exports only visible objects = exactly the one we want
#   - Restore visibility afterward
# This only requires a window to exist (always true even in --background).
# =============================================================================

BLENDER_EXPORT_SCRIPT = r'''
import bpy
import json
import math
import os
import re
import shutil
import sys
import traceback

manifest_path = os.environ["ROOSTER_MANIFEST"]
assets_dir    = os.environ["ROOSTER_ASSETS_DIR"]
os.makedirs(assets_dir, exist_ok=True)

scene     = bpy.context.scene
depsgraph = bpy.context.evaluated_depsgraph_get()


# ---------------------------------------------------------------------------
# Coordinate conversion: Blender Z-up → Rooster Y-up right-handed
# Blender (x, y, z) → Rooster (x, z, -y)
# Matches OBJ export with forward=-Z, up=Y.
# ---------------------------------------------------------------------------
def b2r(v):
    return [float(v[0]), float(v[2]), float(-v[1])]


# ---------------------------------------------------------------------------
# Compat helpers
# ---------------------------------------------------------------------------
def _has_nodes(datablock):
    """
    Check whether a World or Material has a node tree.
    world.use_nodes / material.use_nodes are deprecated in Blender 6+;
    check node_tree directly to be forward-compatible.
    """
    return getattr(datablock, "node_tree", None) is not None


# ---------------------------------------------------------------------------
# OBJ export: hide-based isolation, no selection operators needed
# ---------------------------------------------------------------------------
def export_obj_blender4(obj, obj_path, all_mesh_objects):
    """
    Export a single mesh object using bpy.ops.wm.obj_export (Blender 4+/5+).

    Temporarily hides every other mesh so the exporter, running with
    export_selected_objects=False, sees only this object.
    Does NOT call select_all / select_set, which require a 3D viewport context.
    """
    # Save and override visibility
    saved = {o: o.hide_viewport for o in all_mesh_objects}
    for o in all_mesh_objects:
        o.hide_viewport = (o is not obj)

    ok = False
    try:
        bpy.ops.wm.obj_export(
            filepath=str(obj_path),
            export_selected_objects=False,   # export visible, not selected
            apply_modifiers=True,
            forward_axis="NEGATIVE_Z",
            up_axis="Y",
            export_materials=True,
            export_triangulated_mesh=True,
            export_normals=True,
            export_uv=True,
            global_scale=1.0,
            export_pbr_extensions=True,
        )
        ok = True
    except Exception as exc:
        print(f"[blend_to_rscn] wm.obj_export failed for {obj.name}: {exc}")
        traceback.print_exc()
    finally:
        # Always restore visibility
        for o, hidden in saved.items():
            o.hide_viewport = hidden

    return ok


def export_obj_blender3(obj, obj_path, all_mesh_objects):
    """
    Export a single mesh object using bpy.ops.export_scene.obj (Blender 3.x).

    Same hide-based isolation. export_scene.obj does check for a window but
    not for a 3D viewport, so it works in background mode when given a
    temp_override that supplies window + screen.
    """
    saved = {o: o.hide_viewport for o in all_mesh_objects}
    for o in all_mesh_objects:
        o.hide_viewport = (o is not obj)

    # Build minimal context override for Blender 3 operators
    wm  = bpy.context.window_manager
    win = wm.windows[0] if wm.windows else None
    override = {}
    if win:
        override = {"window": win, "screen": win.screen}

    ok = False
    try:
        with bpy.context.temp_override(**override):
            bpy.ops.export_scene.obj(
                filepath=str(obj_path),
                use_selection=False,        # export visible, not selected
                use_mesh_modifiers=True,
                axis_forward="-Z",
                axis_up="Y",
                use_materials=True,
                use_triangles=True,
                use_normals=True,
                use_uvs=True,
                global_scale=1.0,
            )
        ok = True
    except Exception as exc:
        print(f"[blend_to_rscn] export_scene.obj failed for {obj.name}: {exc}")
        traceback.print_exc()
    finally:
        for o, hidden in saved.items():
            o.hide_viewport = hidden

    return ok


def export_obj_raw(obj, obj_path):
    """
    Pure-Python bmesh fallback – no operators, no context needed.
    Writes triangulated geometry without materials.
    """
    import bmesh
    eval_obj  = obj.evaluated_get(depsgraph)
    mesh_data = eval_obj.to_mesh()
    if not mesh_data:
        eval_obj.to_mesh_clear()
        return False

    bm = bmesh.new()
    try:
        bm.from_mesh(mesh_data)
        bmesh.ops.triangulate(bm, faces=bm.faces)
        world_mat = obj.matrix_world
        lines = [f"# blend_to_rscn raw fallback\no {obj.name}\n"]
        for v in bm.verts:
            wv = world_mat @ v.co
            rx, ry, rz = b2r(wv)
            lines.append(f"v {rx:.6f} {ry:.6f} {rz:.6f}\n")
        for f in bm.faces:
            idx = " ".join(str(loop.vert.index + 1) for loop in f.loops)
            lines.append(f"f {idx}\n")
    finally:
        bm.free()
        eval_obj.to_mesh_clear()

    try:
        with open(obj_path, "w", encoding="utf-8") as fh:
            fh.writelines(lines)
        return True
    except OSError as exc:
        print(f"[blend_to_rscn] raw OBJ write failed: {exc}")
        return False


# Detect which exporter is available once at startup
def _detect_exporter():
    try:
        bpy.ops.wm.obj_export.__doc__
        return "wm"
    except AttributeError:
        pass
    try:
        bpy.ops.export_scene.obj.__doc__
        return "scene"
    except AttributeError:
        pass
    return "raw"

OBJ_EXPORTER = _detect_exporter()
print(f"[blend_to_rscn] OBJ exporter detected: {OBJ_EXPORTER}")


def export_obj(obj, obj_path, all_mesh_objects):
    if OBJ_EXPORTER == "wm":
        return export_obj_blender4(obj, obj_path, all_mesh_objects)
    elif OBJ_EXPORTER == "scene":
        return export_obj_blender3(obj, obj_path, all_mesh_objects)
    else:
        print(f"[blend_to_rscn] WARNING: no OBJ operator found, using raw bmesh fallback.")
        return export_obj_raw(obj, obj_path)


# ---------------------------------------------------------------------------
# Texture collection
# ---------------------------------------------------------------------------
_MAP_LINE_RE = re.compile(
    r"^(\s*(?:map_Kd|map_Ks|map_Ka|map_Bump|map_bump|bump|map_Kn|norm|map_d)\s+)",
    re.IGNORECASE,
)

def collect_textures():
    """
    Walk all mesh materials, copy/unpack textures into assets_dir.
    Returns dict: original identifier → basename in assets_dir.
    """
    tex_remap = {}

    for obj in scene.objects:
        if obj.type != "MESH":
            continue
        for slot in obj.material_slots:
            mat = slot.material
            if not mat or not _has_nodes(mat):
                continue
            for node in mat.node_tree.nodes:
                if node.type != "TEX_IMAGE" or not node.image:
                    continue
                img = node.image

                if img.packed_file:
                    raw_name = bpy.path.basename(img.filepath) if img.filepath else ""
                    if not raw_name or raw_name in ("<none>", ""):
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
                            print(f"[blend_to_rscn] Unpacked: {raw_name}")
                        except Exception as exc:
                            print(f"[blend_to_rscn] WARNING: unpack failed {img.name}: {exc}")
                    tex_remap[img.name] = raw_name
                    if img.filepath:
                        tex_remap[bpy.path.abspath(img.filepath)] = raw_name
                    continue

                src = bpy.path.abspath(img.filepath)
                if not os.path.isfile(src):
                    print(f"[blend_to_rscn] WARNING: texture not found: {src}")
                    continue
                raw_name = os.path.basename(src)
                dst = os.path.join(assets_dir, raw_name)
                if os.path.normcase(os.path.abspath(src)) != os.path.normcase(os.path.abspath(dst)):
                    try:
                        shutil.copy2(src, dst)
                        print(f"[blend_to_rscn] Copied texture: {raw_name}")
                    except Exception as exc:
                        print(f"[blend_to_rscn] WARNING: copy failed {src}: {exc}")
                tex_remap[src] = raw_name
                tex_remap[os.path.basename(src)] = raw_name
                tex_remap[img.name] = raw_name

    return tex_remap


def patch_mtl(mtl_path, tex_remap):
    if not os.path.isfile(mtl_path) or not tex_remap:
        return
    try:
        with open(mtl_path, "r", encoding="utf-8", errors="replace") as fh:
            lines = fh.readlines()
    except OSError as exc:
        print(f"[blend_to_rscn] WARNING: cannot read MTL {mtl_path}: {exc}")
        return

    patched = []
    for line in lines:
        m = _MAP_LINE_RE.match(line)
        if m:
            prefix = m.group(0)
            tokens = line[len(prefix):].strip().split()
            if tokens:
                orig  = tokens[-1]
                flags = tokens[:-1]
                bare  = os.path.basename(orig)
                new   = (
                    tex_remap.get(orig)
                    or tex_remap.get(os.path.abspath(orig) if not os.path.isabs(orig) else orig)
                    or tex_remap.get(bare)
                )
                if new:
                    bare = new
                flag_str = " ".join(flags)
                line = prefix + (flag_str + " " if flag_str else "") + bare + "\n"
        patched.append(line)

    try:
        with open(mtl_path, "w", encoding="utf-8") as fh:
            fh.writelines(patched)
        print(f"[blend_to_rscn] Patched MTL: {os.path.basename(mtl_path)}")
    except OSError as exc:
        print(f"[blend_to_rscn] WARNING: cannot write MTL {mtl_path}: {exc}")


# ---------------------------------------------------------------------------
# Build manifest
# ---------------------------------------------------------------------------
manifest = {
    "viewport":   [scene.render.resolution_x, scene.render.resolution_y],
    "camera":     None,
    "background": [12, 12, 18],
    "lights":     [],
    "objects":    [],
}

# Camera -----------------------------------------------------------------------
cam_obj = scene.camera
if cam_obj:
    import mathutils
    cam     = cam_obj.data
    loc     = cam_obj.matrix_world.to_translation()
    fwd     = cam_obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 0, -1))
    up      = cam_obj.matrix_world.to_quaternion() @ mathutils.Vector((0,  1,  0))
    look_at = loc + fwd

    aspect = scene.render.resolution_x / max(scene.render.resolution_y, 1)
    if cam.sensor_fit == "VERTICAL":
        fov_v_rad = cam.angle
    else:
        fov_v_rad = 2.0 * math.atan(math.tan(cam.angle / 2.0) / aspect)

    manifest["camera"] = {
        "pos":     b2r(loc),
        "look_at": b2r(look_at),
        "up":      b2r(up),
        "fov":     math.degrees(fov_v_rad),
        "near":    cam.clip_start,
        "far":     cam.clip_end,
    }
else:
    print("[blend_to_rscn] WARNING: no camera in scene – using default.")

# World background -------------------------------------------------------------
world = scene.world
if world and _has_nodes(world):
    for node in world.node_tree.nodes:
        if node.type == "BACKGROUND":
            col = node.inputs["Color"].default_value
            strength_input = node.inputs.get("Strength")
            s = float(strength_input.default_value) if strength_input else 1.0
            manifest["background"] = [
                min(255, int(col[0] * 255 * s)),
                min(255, int(col[1] * 255 * s)),
                min(255, int(col[2] * 255 * s)),
            ]
            break

# Lights -----------------------------------------------------------------------
for obj in scene.objects:
    if obj.type != "LIGHT":
        continue
    import mathutils
    light = obj.data
    loc   = obj.matrix_world.to_translation()
    col   = light.color
    r_    = min(255, int(col[0] * 255))
    g_    = min(255, int(col[1] * 255))
    b_    = min(255, int(col[2] * 255))
    iv    = max(0.01, light.energy / 100.0)

    if light.type == "SUN":
        fwd = obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 0, -1))
        manifest["lights"].append({
            "type":      "dir_light",
            "dir":       b2r(fwd),
            "color":     [r_, g_, b_],
            "intensity": iv,
        })
    elif light.type in ("POINT", "AREA", "SPOT"):
        manifest["lights"].append({
            "type":      "point_light",
            "pos":       b2r(loc),
            "color":     [r_, g_, b_],
            "intensity": iv,
        })

if not manifest["lights"]:
    print("[blend_to_rscn] No lights found – inserting defaults.")
    manifest["lights"].append({
        "type": "dir_light", "dir": [-0.7, -1.0, -0.2],
        "color": [255, 255, 255], "intensity": 0.9,
    })
    manifest["lights"].append({
        "type": "point_light", "pos": [2.0, 3.0, 2.0],
        "color": [255, 230, 200], "intensity": 3.0,
    })

# Textures ---------------------------------------------------------------------
tex_remap = collect_textures()

# Meshes -----------------------------------------------------------------------
mesh_objects = [o for o in scene.objects if o.type == "MESH"]
print(f"[blend_to_rscn] Exporting {len(mesh_objects)} mesh object(s)…")

for obj in mesh_objects:
    safe_name    = bpy.path.clean_name(obj.name).replace(" ", "_")
    obj_filename = safe_name + ".obj"
    obj_path     = os.path.join(assets_dir, obj_filename)

    ok = export_obj(obj, obj_path, mesh_objects)
    if not ok or not os.path.isfile(obj_path):
        print(f"[blend_to_rscn] Skipping {obj.name} – export produced no file.")
        continue

    patch_mtl(obj_path[:-4] + ".mtl", tex_remap)

    manifest["objects"].append({
        "name":     obj.name,
        "obj_file": obj_filename,
        "offset":   [0.0, 0.0, 0.0],
    })
    print(f"[blend_to_rscn] Exported: {obj_filename}")

# Write manifest ---------------------------------------------------------------
with open(manifest_path, "w", encoding="utf-8") as fh:
    json.dump(manifest, fh, indent=2)

print(f"[blend_to_rscn] Manifest written → {manifest_path}")
print(f"[blend_to_rscn] Done: {len(manifest['objects'])} mesh(es), {len(manifest['lights'])} light(s).")
'''


# =============================================================================
# Host-side helpers
# =============================================================================


def find_blender(hint: str | None) -> str:
    if hint:
        if not os.path.isfile(hint):
            sys.exit(f"[blend_to_rscn] ERROR: --blender path not found: {hint}")
        return hint
    found = shutil.which("blender") or shutil.which("blender.exe")
    if found:
        return found
    candidates = [
        r"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 5.0\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.4\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.1\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 4.0\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender 3.6\blender.exe",
        r"C:\Program Files\Blender Foundation\Blender\blender.exe",
        "/Applications/Blender.app/Contents/MacOS/Blender",
        os.path.expanduser("~/Applications/Blender.app/Contents/MacOS/Blender"),
        "/usr/bin/blender",
        "/usr/local/bin/blender",
        "/snap/bin/blender",
        "/flatpak/exports/bin/org.blender.Blender",
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    sys.exit(
        "[blend_to_rscn] ERROR: Blender not found. "
        "Install it or pass --blender /path/to/blender."
    )


def run_blender_export(blend_file: Path, assets_dir: Path, blender_exe: str) -> dict:
    with tempfile.TemporaryDirectory(prefix="rooster_blend_") as tmpdir:
        manifest_path = Path(tmpdir) / "manifest.json"
        script_path = Path(tmpdir) / "blender_export.py"
        script_path.write_text(BLENDER_EXPORT_SCRIPT, encoding="utf-8")

        env = os.environ.copy()
        env["ROOSTER_MANIFEST"] = str(manifest_path)
        env["ROOSTER_ASSETS_DIR"] = str(assets_dir)

        cmd = [
            blender_exe,
            "--background",
            str(blend_file),
            "--python",
            str(script_path),
        ]
        print(f"[blend_to_rscn] Running: {' '.join(cmd)}")

        result = subprocess.run(cmd, env=env)
        if result.returncode != 0:
            sys.exit(
                f"[blend_to_rscn] ERROR: Blender exited with code {result.returncode}."
            )
        if not manifest_path.is_file():
            sys.exit(
                "[blend_to_rscn] ERROR: manifest not written – see Blender output above."
            )

        return json.loads(manifest_path.read_text(encoding="utf-8"))


# =============================================================================
# .rscn assembly
# =============================================================================


def _f3(v: list[float]) -> str:
    return " ".join(f"{x:.4f}" for x in v)


def _i3(v: list[int]) -> str:
    return f"{v[0]} {v[1]} {v[2]}"


def build_rscn(manifest: dict, assets_dir: Path, output_rscn: Path) -> None:
    lines: list[str] = ["ROOSTERSCENEV1", ""]

    vp = manifest.get("viewport", [1280, 720])
    lines.append(f"viewport {vp[0]} {vp[1]}")

    cam = manifest.get("camera")
    if cam:
        lines.append(f"fov {cam['fov']:.2f}")
        lines.append(
            f"camera {_f3(cam['pos'])}   {_f3(cam['look_at'])}   {_f3(cam['up'])}"
            f"   {cam['near']:.4f} {cam['far']:.1f}"
        )
    else:
        lines += [
            "fov 60.00",
            "camera 0.0000 3.0000 6.0000   0.0000 0.0000 0.0000   0.0000 1.0000 0.0000   0.1000 100.0",
        ]

    bg = manifest.get("background", [12, 12, 18])
    lines.append(f"background {_i3(bg)}")
    lines.append("max_depth 4")
    lines.append("")
    lines.append("# fallback material – real colours come from .mtl via usemtl")
    lines.append("mat 180 180 190")
    lines.append("")

    for light in manifest.get("lights", []):
        c = light["color"]
        iv = light["intensity"]
        if light["type"] == "dir_light":
            lines.append(f"dir_light {_f3(light['dir'])} {_i3(c)} {iv:.2f}")
        elif light["type"] == "point_light":
            lines.append(f"point_light {_f3(light['pos'])} {_i3(c)} {iv:.2f}")
    lines.append("")

    rscn_dir = output_rscn.parent.resolve()
    assets_resolved = assets_dir.resolve()
    try:
        rel_assets = os.path.relpath(assets_resolved, rscn_dir)
    except ValueError:
        rel_assets = str(assets_resolved)

    for obj in manifest.get("objects", []):
        rel_obj = os.path.join(rel_assets, obj["obj_file"]).replace("\\", "/")
        ox, oy, oz = obj.get("offset", [0.0, 0.0, 0.0])
        lines.append(f"obj {rel_obj} {ox:.4f} {oy:.4f} {oz:.4f} 0")

    output_rscn.parent.mkdir(parents=True, exist_ok=True)
    output_rscn.write_text("\n".join(lines) + "\n", encoding="utf-8")

    n_obj = len(manifest.get("objects", []))
    n_light = len(manifest.get("lights", []))
    print(f"[blend_to_rscn] Written: {output_rscn}")
    print(f"[blend_to_rscn] {n_obj} mesh(es), {n_light} light(s).")


# =============================================================================
# Entry point
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
    parser.add_argument("blend_file", type=Path, help=".blend file to convert")
    parser.add_argument(
        "assets_dir", type=Path, help="directory for OBJ/MTL/texture output"
    )
    parser.add_argument("output_rscn", type=Path, help="path for the generated .rscn")
    parser.add_argument(
        "--blender",
        metavar="EXE",
        default=None,
        help="path to the blender executable (auto-detected if omitted)",
    )
    args = parser.parse_args()

    blend_file = args.blend_file.resolve()
    assets_dir = args.assets_dir.resolve()
    output_rscn = args.output_rscn.resolve()

    if not blend_file.is_file():
        sys.exit(f"[blend_to_rscn] ERROR: blend file not found: {blend_file}")

    blender_exe = find_blender(args.blender)
    print(f"[blend_to_rscn] Blender: {blender_exe}")
    assets_dir.mkdir(parents=True, exist_ok=True)

    print("[blend_to_rscn] Step 1/2 – Exporting from Blender …")
    manifest = run_blender_export(blend_file, assets_dir, blender_exe)

    print("[blend_to_rscn] Step 2/2 – Assembling .rscn …")
    build_rscn(manifest, assets_dir, output_rscn)

    print(
        f"\nDone. Render with:\n    ./build/<platform>/release/cpp-raytracer {output_rscn}"
    )


if __name__ == "__main__":
    main()
