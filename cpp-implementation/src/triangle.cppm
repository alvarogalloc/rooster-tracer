export module triangle;
import vec3;
import object3d;
import std;
import ray;
import hitevent;
import interval;
import color_rgb;

export namespace cg {
struct triangle : object3d {
  triangle(vec3 p0, vec3 p1, vec3 p2, color_rgb c)
      : p0(p0), p1(p1), p2(p2), color_(c) {}
  vec3 p0;
  vec3 p1;
  vec3 p2;
  color_rgb color_;
  // color_rgb color() const override { return color_; }
  std::optional<hitevent> get_hit(ray, interval) const override;
};

} // namespace cg
