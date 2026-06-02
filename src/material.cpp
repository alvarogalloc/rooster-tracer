
module material;

namespace cg
{

[[nodiscard]] color_rgb phong_brdf(const material& m, const hitevent& he,
                                   vec3 light_dir, color_rgb radiance,
                                   vec3 view_dir)
{
    constexpr static double epsilon = 1e-8;
    const vec3 n = safe_normalize(he.normal, epsilon);
    const vec3 l = safe_normalize(light_dir, epsilon);
    const vec3 v = safe_normalize(view_dir, epsilon);
    const double lambert = std::max(0., glm::dot(n, l));
    const vec3 reflection = glm::reflect(-l, n);
    const double spec_base = std::max(0., glm::dot(v, reflection));

    // Scale diffuse reflection by (1.0 - metalness)
    const color_rgb active_diffuse = m.diffuse * (1.0 - m.metalness);

    // Specular highlight color is colored by base color for metals
    const color_rgb active_specular =
        color_rgb{glm::mix(static_cast<vec3>(m.specular),
                           static_cast<vec3>(m.diffuse), m.metalness)};

    // Compute shininess exponent from roughness
    const double active_shininess =
        2.0 / (m.roughness * m.roughness + 1e-4) - 2.0;
    const double exponent = std::max(1., active_shininess);

    const double specular = std::pow(spec_base, exponent);
    return color_rgb{radiance *
                     (active_diffuse * lambert + active_specular * specular)};
}

[[nodiscard]] material make_phong_material(color_rgb albedo, double shininess)
{
    return material{
        .ambient = color_rgb{albedo * kDefaultAmbientFactor},
        .specular = color_rgb{0.5, 0.5, 0.5},
        .diffuse = albedo,
        .shininess = shininess,
        .texture_id = std::nullopt,
    };
}

[[nodiscard]] material make_textured_material(std::size_t texture_id,
                                              color_rgb specular,
                                              double shininess)
{
    return material{
        .ambient = color_rgb{1., 1., 1.} * kDefaultAmbientFactor,
        .specular = specular,
        .diffuse = color_rgb{1., 1., 1.},
        .shininess = shininess,
        .texture_id = texture_id,
    };
}

} // namespace cg
