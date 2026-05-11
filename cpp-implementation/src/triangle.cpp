module triangle;
import glm;
import interval;

namespace cg
{
std::optional<hitevent> get_ray_triangle_hit(const triangle& tt, ray r,
                                             interval i)
{
  const auto& [p0, p1, p2] = tt;
  using glm::mat3;
  const auto col0 = -r.dir;
  const auto col1 = p1 - p0;
  const auto col2 = p2 - p0;
  const auto col3 = r.pos - p0;

  const auto d0 = glm::determinant(mat3{col0, col1, col2});
  const interval close_to_zero{-1e-4, 1e-4};
  if (close_to_zero.contains(d0)) // paralelo al triangulo
    return std::nullopt;

  const auto dt = glm::determinant(mat3{col3, col1, col2});
  const float t = dt / d0;
  if (t < 0.f || !i.contains(t)) // esta detras del rayo
    return std::nullopt;

  const auto du = glm::determinant(mat3{col0, col3, col2});
  const interval zero_to_one{0.f, 1.f};
  const float u = du / d0;
  if (!zero_to_one.contains(u))
    return std::nullopt;

  const auto dv = glm::determinant(mat3{col0, col1, col3});
  const interval v_range{0.f, 1.f - u};
  const float bary_v = dv / d0;
  if (!v_range.contains(bary_v))
    return std::nullopt;

  hitevent hit;
  hit.t = t;
  hit.p = r.at(t);
  auto normal = glm::normalize(glm::cross(col1, col2));
  if (glm::dot(normal, r.dir) > 0.f)
  {
    normal = -normal;
  }
  hit.normal = vec3{normal.x, normal.y, normal.z};
  return hit;
}

} // namespace cg
