export module material;
import std;
import vec3;
import color_rgb;
import directional_light;
import point_light;
import hitevent;

export namespace cg
{
struct material
{
  color_rgb ambient;
  color_rgb specular;
  color_rgb diffuse;
  float shininess = 1.0f;
};

namespace detail
{
constexpr float kDefaultShininess = 32.f;
constexpr float kDefaultAmbientFactor = 0.1f;
constexpr float kMinDistanceSq = 1e-4f;
constexpr float kMinNormSq = 1e-8f;

[[nodiscard]] vec3 safe_normalize(vec3 v)
{
  const float len_sq = glm::dot(v, v);
  if (len_sq <= kMinNormSq)
    return vec3{0.f, 0.f, 0.f};
  return v * glm::inversesqrt(len_sq);
}

[[nodiscard]] color_rgb phong_brdf(const material& m, const hitevent& he,
                                   vec3 light_dir, color_rgb radiance,
                                   vec3 view_dir)
{
  const vec3 n = safe_normalize(he.normal);
  const vec3 l = safe_normalize(light_dir);
  const vec3 v = safe_normalize(view_dir);
  const float lambert = std::max(0.f, glm::dot(n, l));
  const vec3 reflection = glm::reflect(-l, n);
  const float spec_base = std::max(0.f, glm::dot(v, reflection));
  const float specular = std::pow(spec_base, std::max(1.f, m.shininess));
  return color_rgb{radiance * (m.diffuse * lambert + m.specular * specular)};
}
} // namespace detail

[[nodiscard]] material make_phong_material(color_rgb albedo,
                                           float shininess = detail::kDefaultShininess)
{
  return material{
      .ambient = color_rgb{albedo * detail::kDefaultAmbientFactor},
      .specular = color_rgb{1.f, 1.f, 1.f},
      .diffuse = albedo,
      .shininess = shininess,
  };
}

[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const directional_light& l, vec3 view_dir)
{
  return detail::phong_brdf(m, he, -l.dir, l.radiance(), view_dir);
}

[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const point_light& l, vec3 view_dir)
{
  const vec3 to_light = l.pos - he.p;
  const float dist_sq =
      std::max(detail::kMinDistanceSq, glm::dot(to_light, to_light));
  const color_rgb radiance = color_rgb{l.color * (l.intensity / dist_sq)};
  return detail::phong_brdf(m, he, to_light, radiance, view_dir);
}

using material_collection = std::vector<material>;
} // namespace cg
