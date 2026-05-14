export module plane;
import glm;
import std;
import hitevent;
import interval;
import ray;

export namespace cg
{
struct plane
{
  vec3 normal;
  vec3 point;
  std::size_t material_id;
};

std::optional<hitevent> get_ray_plane_hit(const plane&, ray, interval);

} // namespace cg
