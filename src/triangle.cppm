export module triangle;
import glm;
import std;
import ray;
import hitevent;
import interval;
import color_rgb;

export namespace cg
{

struct vertex
{
    vec3 p;
    vec3 n{0., 0., 0.};
    vec2 uv{0., 0.};
};
struct triangle
{
    std::size_t vertex_start;
    std::size_t material_id{0};
};

std::optional<hitevent> get_ray_triangle_hit(const triangle&,
                                             std::span<const vertex> vertices,
                                             ray, interval,
                                             bool cull_backfaces = false);

} // namespace cg
