export module directional_light;
import color_rgb;
import glm;
export namespace cg {
  using namespace glm;

struct directional_light {
  directional_light(vec3 d, color_rgb c, float i)
      : dir(normalize(d)), color(c), intensity(i) {}
  [[nodiscard]] auto radiance() const { return color * intensity; }
  vec3 dir;
  color_rgb color;
  float intensity;
};
} // namespace cg
