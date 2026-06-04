module spot_light;
import color_rgb;
import glm;
import interval;
import ray;
import hitevent;
import material;

namespace cg
{
[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const spot_light& l, vec3 view_dir)
{
    constexpr static double kMinDistanceSq = 1e-7f;

    const vec3 to_light = l.pos - he.p;
    const double dist_sq =
        glm::max(kMinDistanceSq, glm::dot(to_light, to_light));
    const double dist = glm::sqrt(dist_sq);
    const vec3 light_dir = to_light / dist;

    // Spotlight direction attenuation
    const double cos_theta = glm::dot(-light_dir, l.dir);
    double spot_attenuation = 0.0;
    if (cos_theta >= l.cos_angle)
    {
        spot_attenuation = 1.0;
    }

    if (spot_attenuation <= 0.0)
        return color_rgb{0.0, 0.0, 0.0};

    const color_rgb radiance = color_rgb{l.color * (l.intensity / dist_sq) * spot_attenuation};
    return phong_brdf(m, he, to_light, radiance, view_dir);
}

auto get_shadow_info(const spot_light& light, const hitevent& hit,
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
