export module scene_parser;
import std;
import scene;
export namespace cg {
  scene::object_collection parse_scene(const std::string &filepath);
}
