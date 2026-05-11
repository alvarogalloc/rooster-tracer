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
  vec3 n0{0.f, 0.f, 0.f};
  vec3 n1{0.f, 0.f, 0.f};
  vec3 n2{0.f, 0.f, 0.f};
  bool has_vertex_normals{false};
  std::size_t material_id{0};
};

std::optional<hitevent> get_ray_triangle_hit(const triangle&, ray, interval);
} // namespace cg
