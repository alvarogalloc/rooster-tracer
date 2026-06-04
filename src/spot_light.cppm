export module spot_light;
import color_rgb;
import glm;
import hitevent;
import std;
import interval;
import ray;
import material;

export namespace cg
{
struct spot_light
{
    spot_light(vec3 p, vec3 d, color_rgb c, double i, double angle_deg, double r = 0.0, int s = 1)
        : pos(p), dir(glm::normalize(d)), color(c), intensity(i),
          cos_angle(glm::cos(glm::radians(angle_deg / 2.0))),
          radius(r), samples(s)
    {
    }

    vec3 pos;
    vec3 dir;
    color_rgb color;
    double intensity;
    double cos_angle;
    double radius;
    int samples;
};

[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const spot_light& l, vec3 view_dir);

auto get_shadow_info(const spot_light& light, const hitevent& hit,
                     const double kShadowBias) -> std::pair<ray, interval>;

} // namespace cg
