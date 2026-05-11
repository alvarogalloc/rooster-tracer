export module scene;
import std;

import light;
import material;
import triangle;
import sphere;
import mesh3d;
import plane;
export namespace cg
{

struct scene
{
  using primitive_t = std::variant<sphere, triangle, plane, mesh3d>;
  std::vector<primitive_t> objects{};
  light_collection lights{};
  material_collection materials{};
  std::vector<triangle> triangles{};
};
} // namespace cg
