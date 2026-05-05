
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
                const directional_light &l, vec3)  {
  // ambient
  color_rgb result = l.color;
  // emission
  // result += m.emission;

  // shadow check placeholder — you'll pass a scene/BVH later
  // if (in_shadow(he.p, l, scene)) return result;

  float lambert = std::max(0.0f, glm::dot(he.normal, -l.dir));
  if (lambert <= 0.0f)
    return result;

  result += l.radiance() * m.albedo * lambert;

  return result;
}
using material_collection = std::vector<material>;
} // namespace cg
