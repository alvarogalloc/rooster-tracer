
module material;

namespace cg
{
namespace
{

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
} // namespace

[[nodiscard]] material make_phong_material(color_rgb albedo, float shininess)
{
    return material{
        .ambient = color_rgb{albedo * kDefaultAmbientFactor},
        .specular = color_rgb{1.f, 1.f, 1.f},
        .diffuse = albedo,
        .shininess = shininess,
        .texture_id = std::nullopt,
    };
}

[[nodiscard]] material make_textured_material(std::size_t texture_id,
                                              color_rgb specular,
                                              float shininess)
{
    return material{
        .ambient = color_rgb{1.f, 1.f, 1.f} * kDefaultAmbientFactor,
        .specular = specular,
        .diffuse = color_rgb{1.f, 1.f, 1.f},
        .shininess = shininess,
        .texture_id = texture_id,
    };
}

[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const directional_light& l, vec3 view_dir)
{
    return phong_brdf(m, he, -l.dir, l.radiance(), view_dir);
}

[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const point_light& l, vec3 view_dir)
{
    const vec3 to_light = l.pos - he.p;
    const float dist_sq =
        std::max(kMinDistanceSq, glm::dot(to_light, to_light));
    const color_rgb radiance = color_rgb{l.color * (l.intensity / dist_sq)};
    return phong_brdf(m, he, to_light, radiance, view_dir);
}

} // namespace cg
