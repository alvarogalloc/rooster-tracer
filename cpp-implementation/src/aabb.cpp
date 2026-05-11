module aabb;
import std;
import hitevent;
import interval;
import ray;
import vec3;

namespace cg
{

aabb compute_span_aabb(std::span<const triangle> vertices,
                       std::span<const std::uint32_t> indices)
{
  auto inf = std::numeric_limits<float>::infinity();
  aabb box{.min = {inf, inf, inf}, .max = {-inf, -inf, -inf}};

  for (const auto i : indices)
  {
    const auto [a, b, c] = vertices.at(i);
    for (const auto v : {a, b, c})
    {

      box.min.x = std::min(box.min.x, v.x);
      box.min.y = std::min(box.min.y, v.y);
      box.min.z = std::min(box.min.z, v.z);

      box.max.x = std::max(box.max.x, v.x);
      box.max.y = std::max(box.max.y, v.y);
      box.max.z = std::max(box.max.z, v.z);
    }
  }

  return box;
}

bool is_ray_aabb_hit(const aabb& box, ray r, interval valid)
{
  // planes and boxes are related
  // the boxes can be represented a many planes in a given xy xz or yz domain
  // there are two intersections and we are going to take the closest one, i.e.
  // min t we can say that the ray entered the box if it intersects any two
  // different planes
  const vec3 inv_dir = 1.0f / r.dir;

  const vec3 t_near = (box.min - r.pos) * inv_dir;
  const vec3 t_far = (box.max - r.pos) * inv_dir;

  const vec3 t_min = glm::min(t_near, t_far);
  const vec3 t_max = glm::max(t_near, t_far);

  const float t_enter = glm::compMax(t_min);
  const float t_exit = glm::compMin(t_max);

  return t_enter <= t_exit && valid.contains(t_enter);
}
} // namespace cg
