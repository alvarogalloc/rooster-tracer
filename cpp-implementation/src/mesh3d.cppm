export module mesh3d;
import std;
import ray;
import interval;
import hitevent;
import aabb;
import triangle;
import bvh;

export namespace cg
{

struct mesh3d
{
  std::size_t triangle_start;
  std::size_t triangle_count;
  std::size_t material_id;

  bvh blas;
};

std::optional<cg::hitevent> get_ray_mesh_hit(const mesh3d& mesh,
                                             std::span<const triangle> triangles,
                                             std::span<const vertex> vertices,
                                             ray r, interval valid);
} // namespace cg
