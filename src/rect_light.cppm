export module rect_light;
import color_rgb;
import glm;

export namespace cg
{
struct rect_light
{
    rect_light(vec3 p, vec3 u_vec, vec3 v_vec, color_rgb c, double i, int s)
        : pos(p), u(u_vec), v(v_vec), color(c), intensity(i), samples(s)
    {
        normal = glm::normalize(glm::cross(u_vec, v_vec));
    }

    vec3 pos;
    vec3 u;
    vec3 v;
    color_rgb color;
    double intensity;
    int samples;
    vec3 normal;
};
} // namespace cg
