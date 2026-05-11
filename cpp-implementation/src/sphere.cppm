export module sphere;
import vec3;
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
  // color_rgb color() const override { return color_; }
};

   std::optional<hitevent> get_ray_sphere_hit(const sphere&, ray, interval);
} // namespace cg
