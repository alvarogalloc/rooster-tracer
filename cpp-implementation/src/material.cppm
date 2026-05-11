
export module material;
import std;
import vec3;
import color_rgb;
import directional_light;
import hitevent;
export namespace cg
{

struct material
{
  auto operator<=>(const material&) const = default;
  color_rgb ambient;
  color_rgb specular;
  color_rgb diffuse;
  float shininess = 1.0f;
};
color_rgb shade_flat(const material& m, const hitevent& he,
                     const directional_light& l, vec3)
{
  const vec3 n = glm::normalize(he.normal);
  const float lambert = std::max(0.0f, glm::dot(n, -l.dir));
  return color_rgb{l.radiance() * m.ambient * lambert};
}
using material_collection = std::vector<material>;
} // namespace cg
