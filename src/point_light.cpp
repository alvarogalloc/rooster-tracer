module point_light;

namespace cg
{
[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const point_light& l, vec3 view_dir)
{
    constexpr static double kMinDistanceSq = 1e-7f;

    const vec3 to_light = l.pos - he.p;
    const double dist_sq =
        glm::max(kMinDistanceSq, glm::dot(to_light, to_light));
    const color_rgb radiance = color_rgb{l.color * (l.intensity / dist_sq)};
    return phong_brdf(m, he, to_light, radiance, view_dir);
}

auto get_shadow_info(const point_light& light, const hitevent& hit,
                     const double kShadowBias) -> std::pair<ray, interval>
{
    const vec3 to_light = light.pos - hit.p;
    const double light_distance = glm::length(to_light);
    const vec3 shadow_origin = hit.p + hit.normal * kShadowBias;
    const ray shadow_ray{shadow_origin, to_light};
    if (light_distance <= kShadowBias)
        return {shadow_ray, empty_interval};
    const interval shadow_range{kShadowBias, light_distance - kShadowBias};
    return {shadow_ray, shadow_range};
}
} // namespace cg
