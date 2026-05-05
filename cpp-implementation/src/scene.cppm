export module scene;
import std;

import object3d;
import light;
import material;
export namespace cg {

struct scene {
  object_collection objects{};
  light_collection lights{};
  material_collection materials{};
};
} // namespace cg
