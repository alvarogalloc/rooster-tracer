
export module aabb;
import std;
import hitevent;
import interval;
import ray;
import vec3;

export namespace cg
{

struct aabb
{
  vec3 min;
  vec3 max;
};


aabb compute_aabb(std::span<const vec3> vertices)
{
  auto inf = std::numeric_limits<float>::infinity();
  aabb box{ .min = { inf,  inf,  inf },
            .max = {-inf, -inf, -inf } };

  for (const auto& v : vertices)
  {
    box.min.x = std::min(box.min.x, v.x);
    box.min.y = std::min(box.min.y, v.y);
    box.min.z = std::min(box.min.z, v.z);

    box.max.x = std::max(box.max.x, v.x);
    box.max.y = std::max(box.max.y, v.y);
    box.max.z = std::max(box.max.z, v.z);
  }

  return box;
}


std::optional<hitevent> get_hit(const aabb&, ray, interval)
{
  return std::nullopt;
}
} // namespace cg
