export module directional_light;
import color_rgb;
import glm;
import std;
import interval;
import ray;

import hitevent;
import material;
export namespace cg
{
struct directional_light
{
    directional_light(vec3 d, color_rgb c, double i)
        : dir(glm::normalize(d)), color(c), intensity(i)
    {
    }
    [[nodiscard]] auto radiance() const
    {
        return color * intensity;
    }
    vec3 dir;
    color_rgb color;
    double intensity;
};
[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const directional_light& l, vec3 view_dir);

auto get_shadow_info(const directional_light& light, const hitevent& hit,
                     double kShadowBias) -> std::pair<ray, interval>;

} // namespace cg
