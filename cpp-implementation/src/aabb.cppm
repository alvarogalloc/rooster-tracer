
export module aabb;
import std;
import hitevent;
import interval;
import triangle;
import ray;
import vec3;

export namespace cg
{

struct aabb
{
  vec3 min;
  vec3 max;
};

static_assert(sizeof(aabb) == 24, "aabb has unexpected padding");
aabb compute_span_aabb(std::span<const triangle> triangles,std::span<const std::uint32_t>indices);
bool is_ray_aabb_hit(const aabb&, ray, interval);
} // namespace cg
