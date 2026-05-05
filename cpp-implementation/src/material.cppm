
export module material;
import std;
import vec3;
import color_rgb;
import directional_light;
import hitevent;
export namespace cg {

struct material {
  color_rgb albedo;
  // float roughness     = 1.0f;
  // float metallic      = 0.0f;
};
color_rgb shade(const material &m, const hitevent &he,
                const directional_light &l, vec3) {
  const vec3 n = glm::normalize(he.normal);
  const float lambert = std::max(0.0f, glm::dot(n, -l.dir));
  return color_rgb{l.radiance() * m.albedo * lambert};
}
using material_collection = std::vector<material>;
} // namespace cg
