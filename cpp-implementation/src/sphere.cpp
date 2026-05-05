module sphere;
import std;
namespace cg {

std::optional<hitevent> sphere::get_hit(ray r, interval i) const {
  const auto dir_to_sphere = this->pos - r.pos;
  auto a = glm::length2(r.dir);
  auto h = glm::dot(r.dir, dir_to_sphere);
  auto c = glm::length2(dir_to_sphere) - radius * radius;

  auto discriminant = h * h - a * c;

  if (discriminant < 0.f) {
    return std::nullopt;
  }

  float t = (h - std::sqrt(discriminant)) / a;
  if (t < 0 || !i.contains(t)) {
    return std::nullopt;
  }
  return hitevent{t, r.at(t), (r.at(t) - pos) / radius};
}

} // namespace cg
