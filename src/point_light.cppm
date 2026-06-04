export module point_light;
import color_rgb;
import glm;
import hitevent;
import std;
import interval;
import ray;

import material;

export namespace cg
{
struct point_light
{
    point_light(vec3 p, color_rgb c, double i, double r = 0.0, int s = 1)
        : pos(p), color(c), intensity(i), radius(r), samples(s)
    {
    }

    vec3 pos;
    color_rgb color;
    double intensity;
    double radius;
    int samples;
};
[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const point_light& l, vec3 view_dir);

auto get_shadow_info(const point_light& light, const hitevent& hit,
                     const double kShadowBias) -> std::pair<ray, interval>;

} // namespace cg
