export module sphere;
import vec3;
import color_rgb;
import std;
import object3d;
import hitevent;
import ray;

export namespace cg {
struct sphere : object3d {
  sphere(float r, vec3 v, color_rgb col) : radius(r), pos(v), color_(col) {}
  float radius;
  vec3 pos;
  color_rgb color_;
  std::optional<hitevent> get_hit(ray) const override;
  color_rgb color() const override { return color_; }
};

} // namespace cg
