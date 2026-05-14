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
import camera;
export namespace cg
{
struct scene
{
  using primitive_t = std::variant<sphere, triangle, plane, mesh3d>;
  std::vector<primitive_t> objects{};
  light_collection lights{};
  material_collection materials{};
  std::vector<triangle> mesh_triangles{};
  std::string source_dir{};
  camera camera_data{};
  color_rgb background_color{color_rgb::from_rgb_256(10, 32, 90)};
  u32 max_depth{5};
};
} // namespace cg
