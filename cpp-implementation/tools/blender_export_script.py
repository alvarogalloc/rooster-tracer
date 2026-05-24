"""
blender_export_script.py – Runs inside Blender's Python interpreter.
Launched by blend_to_rscn.py via: blender --background file.blend --python this_script.py

Environment variables read:
  ROOSTER_MANIFEST    - path to write the JSON manifest
  ROOSTER_ASSETS_DIR  - directory to write OBJ/MTL/texture files into
"""
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

    # Blender camera local axes:
    #   -Z = forward (camera looks down -Z in local space)
    #   +Y = up
    fwd     = cam_obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 0, -1))
    up      = cam_obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 1, 0))
    look_at = loc + fwd

    near = cam.clip_start
    far  = cam.clip_end

    # Coordinate conversion: Blender Z-up → raytracer Y-up
    #   b2r([x, y, z]) = [x, z, -y]
    def b2r(v):
        return [v.x, v.z, -v.y]

    # FOV conversion -----------------------------------------------------------
    # cam.angle is *horizontal* FOV for AUTO/HORIZONTAL sensor_fit (default for
    # landscape renders), *vertical* for VERTICAL.  The raytracer's screen_to_ndc
    # applies tan(fov/2) on the Y (vertical) axis, so we always export vertical FOV.
    aspect    = scene.render.resolution_x / max(scene.render.resolution_y, 1)
    sensor_fit = cam.sensor_fit   # 'AUTO', 'HORIZONTAL', or 'VERTICAL'
    if sensor_fit == 'VERTICAL':
        fov_v_rad = cam.angle
    else:
        h_fov     = cam.angle
        fov_v_rad = 2.0 * math.atan(math.tan(h_fov / 2.0) / aspect)
    fov_deg = math.degrees(fov_v_rad)
    # --------------------------------------------------------------------------

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
            col      = node.inputs["Color"].default_value
            strength = node.inputs.get("Strength", None)
            s        = strength.default_value if strength else 1.0
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
    loc   = obj.matrix_world.to_translation()
    col   = light.color
    r = min(255, int(col[0] * 255))
    g = min(255, int(col[1] * 255))
    b = min(255, int(col[2] * 255))
    intensity = light.energy / 100.0   # rough normalisation

    if light.type == "SUN":
        import mathutils
        fwd       = obj.matrix_world.to_quaternion() @ mathutils.Vector((0, 0, -1))
        dx, dy, dz = b2r_plain(fwd.x, fwd.y, fwd.z)
        manifest["lights"].append({
            "type":      "dir_light",
            "dir":       [dx, dy, dz],
            "color":     [r, g, b],
            "intensity": max(0.1, intensity),
        })
    elif light.type in ("POINT", "AREA", "SPOT"):
        px, py, pz = b2r_plain(loc.x, loc.y, loc.z)
        manifest["lights"].append({
            "type":      "point_light",
            "pos":       [px, py, pz],
            "color":     [r, g, b],
            "intensity": max(0.1, intensity),
        })

# Fall back to a default sun + fill light if the scene has none
if not manifest["lights"]:
    manifest["lights"].append({
        "type":      "dir_light",
        "dir":       [-0.7, -1.0, -0.2],
        "color":     [255, 255, 255],
        "intensity": 0.9,
    })
    manifest["lights"].append({
        "type":      "point_light",
        "pos":       [2.0, 3.0, 2.0],
        "color":     [255, 230, 200],
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

for obj in mesh_objects:
    # Evaluate modifiers
    eval_obj  = obj.evaluated_get(depsgraph)
    mesh_data = eval_obj.to_mesh()
    if not mesh_data or not mesh_data.vertices:
        eval_obj.to_mesh_clear()
        continue

    safe_name    = bpy.path.clean_name(obj.name).replace(" ", "_")
    obj_filename = safe_name + ".obj"
    obj_path     = os.path.join(assets_dir, obj_filename)
    mtl_path     = obj_path[:-4] + ".mtl"

    # Background-mode-safe export: temporarily hide every other mesh so that
    # export_selected_objects=False still only writes this one object.
    # (bpy.ops.object.select_all / select_set require a 3D viewport context.)
    hidden = {}
    for other in mesh_objects:
        if other is not obj:
            hidden[other.name] = other.hide_render
            other.hide_render  = True

    # Export OBJ + MTL
    export_ok = False
    try:
        # Blender 4.x / 5.x
        bpy.ops.wm.obj_export(
            filepath=obj_path,
            export_selected_objects=False,
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
            # Blender 3.x
            bpy.ops.export_scene.obj(
                filepath=obj_path,
                use_selection=False,
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

    # Restore visibility
    for other in mesh_objects:
        if other.name in hidden:
            other.hide_render = hidden[other.name]

    eval_obj.to_mesh_clear()

    if not export_ok:
        continue

    # -------------------------------------------------------------------------
    # Collect & copy textures
    # Walk all material slots, find Image Texture nodes, unpack/copy their
    # files to assets_dir so the scene is fully self-contained.
    # -------------------------------------------------------------------------
    tex_remap = {}   # source path / image name  →  bare filename in assets_dir

    for slot in obj.material_slots:
        mat = slot.material
        if not mat or not mat.use_nodes:
            continue
        for node in mat.node_tree.nodes:
            if node.type != "TEX_IMAGE" or not node.image:
                continue
            img = node.image

            if img.packed_file:
                # Image is packed inside the .blend – save it to disk first
                raw_name = bpy.path.basename(img.filepath) if img.filepath else img.name
                if not raw_name or raw_name == "<none>":
                    raw_name = img.name
                # Make sure the filename has a recognised extension
                if not any(raw_name.lower().endswith(ext)
                           for ext in (".png", ".jpg", ".jpeg", ".tga", ".bmp", ".exr", ".hdr")):
                    raw_name += ".png"
                save_path = os.path.join(assets_dir, raw_name)
                try:
                    old_fp          = img.filepath_raw
                    img.filepath_raw = save_path
                    img.save()
                    img.filepath_raw = old_fp
                    print(f"[blend_to_rscn] Unpacked image: {raw_name}")
                    tex_remap[img.name] = raw_name
                except Exception as exc:
                    print(f"[blend_to_rscn] WARNING: could not unpack {img.name}: {exc}")
                continue

            # External image – resolve absolute path and copy
            src = bpy.path.abspath(img.filepath)
            if not os.path.isfile(src):
                print(f"[blend_to_rscn] WARNING: texture not found: {src}")
                continue
            raw_name = os.path.basename(src)
            dst      = os.path.join(assets_dir, raw_name)
            if os.path.abspath(src) != os.path.abspath(dst):
                shutil.copy2(src, dst)
                print(f"[blend_to_rscn] Copied texture: {raw_name}")
            tex_remap[src] = raw_name

    # Patch the .mtl: rewrite every map_* line to use only the bare filename,
    # so the raytracer finds textures next to the .mtl regardless of where the
    # original image files lived.
    if os.path.isfile(mtl_path) and tex_remap:
        with open(mtl_path, "r", encoding="utf-8", errors="replace") as fh:
            mtl_lines = fh.readlines()
        patched = []
        for line in mtl_lines:
            m = _MAP_KW_RE.match(line)
            if m:
                rest    = line[len(m.group(0)):].strip()
                tokens  = rest.split()
                # tokens[-1] is the path (flags like -bm come before it)
                orig    = tokens[-1] if tokens else ""
                bare    = os.path.basename(orig)
                # See if we remapped this file
                for old_key, new_name in tex_remap.items():
                    if os.path.basename(old_key) == bare or old_key == orig:
                        bare = new_name
                        break
                # Reconstruct line: keep any flags, replace the path token
                flags = " ".join(tokens[:-1])
                line  = m.group(0) + (flags + " " if flags else "") + bare + "\n"
            patched.append(line)
        with open(mtl_path, "w", encoding="utf-8") as fh:
            fh.writelines(patched)
        print(f"[blend_to_rscn] Patched texture paths in: {os.path.basename(mtl_path)}")

    manifest["objects"].append({
        "name":     obj.name,
        "obj_file": obj_filename,
        "offset":   [0.0, 0.0, 0.0],
    })

# ---------------------------------------------------------------------------
# Write manifest
# ---------------------------------------------------------------------------
with open(manifest_path, "w") as f:
    json.dump(manifest, f, indent=2)

print(f"[blend_to_rscn] Manifest written to {manifest_path}")
print(f"[blend_to_rscn] Exported {len(manifest['objects'])} mesh object(s).")
