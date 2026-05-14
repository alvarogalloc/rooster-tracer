export module scene;
import std;

import light;
import material;
import triangle;
import sphere;
import mesh3d;
import glm;
import color_rgb;
import plane;
export namespace cg
{
struct render_settings
{
  int image_width{1280};
  int image_height{720};
  float fov_radians{1.0472f};
  vec3 camera_pos{0.f, 2.f, 5.f};
  vec3 camera_up{0.f, 1.f, 0.f};
  vec3 camera_look_at{0.f, -1.f, -4.f};
  float near_plane{0.001f};
  float far_plane{20.f};
  color_rgb background{color_rgb::from_rgb_256(10, 32, 90)};
  u32 max_depth{5};
};

struct scene
{
  using primitive_t = std::variant<sphere, triangle, plane, mesh3d>;
  std::vector<primitive_t> objects{};
  light_collection lights{};
  material_collection materials{};
  std::vector<triangle> mesh_triangles{};
  std::string source_dir{};
  render_settings settings{};
};
} // namespace cg
