#!/usr/bin/env python3
"""
blend_to_rscn.py – Convert a Blender .blend scene to a Rooster .rscn scene file.

Usage:
    python tools/blend_to_rscn.py <scene.blend> <assets_dir> <output.rscn> [--blender <blender_exe>]

Steps:
    1. Launches Blender in background mode and runs an embedded export script.
       The export script dumps camera, lights, and mesh objects to a temporary
       JSON manifest plus individual .obj files into <assets_dir>.
    2. Reads the manifest and assembles a .rscn file.

Requirements:
    - Blender 3.x / 4.x accessible on PATH (or via --blender argument).
    - Python 3.10+ (standard library only; no third-party packages needed here).
"""

from __future__ import annotations

import argparse
import json
import math
import textwrap
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# Path to the Blender-side export script (lives next to this file)
_SCRIPT_DIR = Path(__file__).parent
BLENDER_EXPORT_SCRIPT_PATH = _SCRIPT_DIR / "blender_export_script.py"



# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def find_blender(hint: str | None) -> str:
    """Return path to the blender executable."""
    if hint:
        return hint
    # Try common names on PATH
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
    if not BLENDER_EXPORT_SCRIPT_PATH.exists():
        print(f"[blend_to_rscn] ERROR: export script not found: {BLENDER_EXPORT_SCRIPT_PATH}",
              file=sys.stderr)
        sys.exit(1)

    with tempfile.TemporaryDirectory(prefix="rooster_blend_") as tmpdir:
        manifest_path = Path(tmpdir) / "manifest.json"

        env = os.environ.copy()
        env["ROOSTER_MANIFEST"]   = str(manifest_path)
        env["ROOSTER_ASSETS_DIR"] = str(assets_dir)

        cmd = [
            blender_exe,
            "--background",
            str(blend_file),
            "--python", str(BLENDER_EXPORT_SCRIPT_PATH),
        ]

        print(f"[blend_to_rscn] Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, env=env, capture_output=False)
        if result.returncode != 0:
            print(f"[blend_to_rscn] Blender exited with code {result.returncode}.",
                  file=sys.stderr)
            sys.exit(1)

        if not manifest_path.exists():
            print("[blend_to_rscn] ERROR: manifest not written by Blender script.",
                  file=sys.stderr)
            sys.exit(1)

        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    return manifest


# ---------------------------------------------------------------------------
# .rscn assembly
# ---------------------------------------------------------------------------

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
        pos     = cam["pos"]
        look_at = cam["look_at"]
        up      = cam.get("up", [0, 1, 0])
        near    = cam.get("near", 0.1)
        far     = cam.get("far", 100.0)
        lines.append(
            f"camera {vec3_str(pos)}   {vec3_str(look_at)}   {vec3_str(up)}   {near:.3f} {far:.1f}"
        )
    else:
        # sensible fallback
        lines.append("fov 60")
        lines.append("camera 0 3 6   0 0 0   0 1 0   0.1 100")

    # -- Background
    bg = manifest.get("background", [12, 12, 18])
    lines.append(f"background {bg[0]} {bg[1]} {bg[2]}")

    # -- Render depth
    lines.append("max_depth 2")
    lines.append("")

    # -- Default fallback material (used when an OBJ has no usemtl directive)
    # Real materials come from the .mtl files exported alongside each .obj.
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
    # The .rscn obj path must be relative to the .rscn file's directory.
    rscn_dir = output_rscn.parent.resolve()
    assets_dir_resolved = assets_dir.resolve()
    try:
        rel_assets = os.path.relpath(assets_dir_resolved, rscn_dir)
    except ValueError:
        # Different drives on Windows – fall back to absolute path
        rel_assets = str(assets_dir_resolved)

    for obj in manifest.get("objects", []):
        obj_file = obj["obj_file"]
        offset   = obj.get("offset", [0.0, 0.0, 0.0])
        rel_obj  = os.path.join(rel_assets, obj_file).replace("\\", "/")
        ox, oy, oz = offset
        # fallback mat id is always 0 (the default grey defined above);
        # per-face materials are resolved from the .mtl via usemtl.
        lines.append(f"obj {rel_obj} {ox:.4f} {oy:.4f} {oz:.4f} 0")

    content = "\n".join(lines) + "\n"
    output_rscn.parent.mkdir(parents=True, exist_ok=True)
    output_rscn.write_text(content, encoding="utf-8")
    print(f"[blend_to_rscn] Written: {output_rscn}")
    print(f"[blend_to_rscn] Scene contains {len(manifest.get('objects', []))} mesh(es) and {len(manifest.get('lights', []))} light(s).")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert a Blender .blend scene to a Rooster Raytracer .rscn file.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Example:
                python tools/blend_to_rscn.py \\
                    blenderscenes/samplecube.blend \\
                    assets/from_blender \\
                    scenes/samplecube.rscn
        """),
    )
    parser.add_argument("blend_file",   type=Path, help="Path to the .blend file")
    parser.add_argument("assets_dir",   type=Path, help="Directory for exported OBJ/texture/material files")
    parser.add_argument("output_rscn",  type=Path, help="Path for the generated .rscn file")
    parser.add_argument(
        "--blender", metavar="EXE", default=None,
        help="Path to the Blender executable (auto-detected if not given)"
    )
    args = parser.parse_args()

    blend_file  = args.blend_file.resolve()
    assets_dir  = args.assets_dir.resolve()
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
