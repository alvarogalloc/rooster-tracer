module triangle;
import glm;
import interval;

using namespace cg;
inline auto v(vec3 vec) { return glm::vec3{vec.x, vec.y, vec.z}; }
namespace cg {
std::optional<hitevent> triangle::get_hit(ray r, interval i) const {
  using glm::mat3;
  const auto col0 = v(-1 * r.dir);
  const auto col1 = v(p1 - p0);
  const auto col2 = v(p2 - p0);
  const auto col3 = v(r.pos - p0);

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
  const float v = dv / d0;
  if (!v_range.contains(v))
    return std::nullopt;

  hitevent hit;
  hit.t = t;
  hit.p = r.at(t);
  const auto normal = glm::cross(col1, col2);
  hit.normal = vec3{normal.x, normal.y, normal.z};
  return hit;
}

} // namespace cg
