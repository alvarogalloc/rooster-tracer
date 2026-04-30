export module scene;
import std;

import object3d;
export namespace cg {

struct scene {
  using object_collection = std::vector<std::unique_ptr<object3d>>;
  object_collection objects{};
};
} // namespace cg
