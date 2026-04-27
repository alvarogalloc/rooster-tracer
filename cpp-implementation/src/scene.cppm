export module scene;
import std;

import object3d;
export namespace cg {

struct scene {
  std::vector<std::unique_ptr<object3d>> objects{};
};
} // namespace cg
