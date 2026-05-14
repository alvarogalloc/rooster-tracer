export module sphere;
import glm;
import color_rgb;
import std;
import hitevent;
import interval;
import material;
import ray;

export namespace cg
{
struct sphere 
{
  float radius;
  vec3 pos;
  std::size_t material_id{0};
};

   std::optional<hitevent> get_ray_sphere_hit(const sphere&, ray, interval);
} // namespace cg
