export module mesh3d;
import std;
import ray;
import interval;
import hitevent;
import aabb;

export namespace cg
{

struct mesh3d
{
  std::size_t vertex_start;
  std::size_t vertex_count;
  std::size_t material_id;
  // aabb box;
};

std::optional<hitevent> get_ray_mesh_hit(const mesh3d&, ray, interval)
{
  return std::nullopt;
}
} // namespace cg
