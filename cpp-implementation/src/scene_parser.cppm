export module scene_parser;
import std;
import scene;
import object3d;
export namespace cg {
  scene parse_scene(const std::string &filepath);
}
