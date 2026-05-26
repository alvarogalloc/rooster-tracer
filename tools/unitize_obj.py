#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def parse_vertex(line: str) -> tuple[float, float, float, list[str]]:
    parts = line.split()
    if len(parts) < 4:
        raise ValueError("vertex line missing coordinates")
    x, y, z = (float(parts[1]), float(parts[2]), float(parts[3]))
    tail = parts[4:]
    return x, y, z, tail


def format_vertex(x: float, y: float, z: float, tail: list[str]) -> str:
    coords = f"v {x:.6f} {y:.6f} {z:.6f}"
    if tail:
        return coords + " " + " ".join(tail)
    return coords


def unitize_vertices(vertices: list[tuple[float, float, float]]) -> tuple[list[tuple[float, float, float]], float, float, float]:
    xs = [v[0] for v in vertices]
    ys = [v[1] for v in vertices]
    zs = [v[2] for v in vertices]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    min_z, max_z = min(zs), max(zs)
    center_x = (min_x + max_x) * 0.5
    center_y = (min_y + max_y) * 0.5
    center_z = (min_z + max_z) * 0.5
    extent = max(max_x - min_x, max_y - min_y, max_z - min_z)
    scale = 1.0 / extent if extent > 0.0 else 1.0
    unitized = [((x - center_x) * scale, (y - center_y) * scale, (z - center_z) * scale) for x, y, z in vertices]
    return unitized, center_x, center_y, center_z


def unitize_obj(input_path: Path, output_path: Path, scale: float) -> None:
    lines = input_path.read_text().splitlines()
    vertices: list[tuple[float, float, float]] = []
    vertex_tails: list[list[str]] = []
    for line in lines:
        if line.startswith("v "):
            x, y, z, tail = parse_vertex(line)
            vertices.append((x, y, z))
            vertex_tails.append(tail)

    if not vertices:
        raise RuntimeError(f"no vertices found in {input_path}")

    unitized, _, _, _ = unitize_vertices(vertices)
    scaled = [(x * scale, y * scale, z * scale) for x, y, z in unitized]

    output_lines: list[str] = []
    vertex_index = 0
    for line in lines:
        if line.startswith("v "):
            x, y, z = scaled[vertex_index]
            output_lines.append(format_vertex(x, y, z, vertex_tails[vertex_index]))
            vertex_index += 1
        else:
            output_lines.append(line)

    output_path.write_text("\n".join(output_lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Unitize OBJ mesh to unit box centered at origin.")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--scale", type=float, default=1.0, help="Additional scale factor after unitization")
    args = parser.parse_args()
    unitize_obj(args.input, args.output, args.scale)


if __name__ == "__main__":
    main()
