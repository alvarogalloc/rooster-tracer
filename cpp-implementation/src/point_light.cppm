export module point_light;
import color_rgb;
import glm;

export namespace cg
{
struct point_light
{
  point_light(vec3 p, color_rgb c, float i) : pos(p), color(c), intensity(i)
  {
  }

  vec3 pos;
  color_rgb color;
  float intensity;
};
} // namespace cg
