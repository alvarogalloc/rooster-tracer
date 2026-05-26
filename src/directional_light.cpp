
module directional_light;
import color_rgb;
import glm;
namespace cg
{
[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const directional_light& l, vec3 view_dir)
{
    return phong_brdf(m, he, -l.dir, l.radiance(), view_dir);
}

auto get_shadow_info(const directional_light& light, const hitevent& hit,
                     double kShadowBias) -> std::pair<ray, interval>
{
    const vec3 shadow_origin = hit.p + hit.normal * kShadowBias;
    const vec3 light_dir = -light.dir;
    const ray shadow_ray{shadow_origin, light_dir};
    const interval shadow_range{kShadowBias,
                                std::numeric_limits<double>::infinity()};

    return std::pair{shadow_ray, shadow_range};
}

} // namespace cg
