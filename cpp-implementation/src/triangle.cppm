export module triangle;
import vec3;
import std;
import ray;
import hitevent;
import interval;
import color_rgb;

export namespace cg
{
struct triangle
{
  vec3 p0;
  vec3 p1;
  vec3 p2;
};

std::optional<hitevent> get_ray_triangle_hit(const triangle&, ray, interval);
} // namespace cg
