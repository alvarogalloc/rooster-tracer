module scene_parser;
import object3d;
import triangle;
import sphere;
import vec3;
import color_rgb;
import obj_parser;
import common_parsers;

namespace cg {

static const std::unordered_map<
    std::string, void (*)(std::istringstream &, scene::object_collection &)>
    handlers{
        {"sphere", &parsers::parse_sphere},
        {"triangle", &parsers::parse_triangle},
        {"obj", &parsers::parse_obj_file},
    };
scene::object_collection parse_scene(const std::string &filepath) {

  constexpr static auto filetoken = "ROOSTERSCENEV1";
  std::ifstream f{filepath};
  if (not f.is_open()) {
    throw std::runtime_error{
        std::format("scene file ({}) not found", filepath)};
  }
  std::string line;
  std::getline(f, line);
  if (not line.starts_with(filetoken)) {
    std::println(std::cerr, "no file token {} found", filetoken);
  }
  scene::object_collection objs;
  while (std::getline(f, line)) {
    parse_utils::trim_line(line);
    if (parse_utils::should_skip_line(line))
      continue;
    std::istringstream ss(line);
    std::string type;
    ss >> type;
    if (!handlers.count(type)) {
      std::println(std::cerr, "unknown type of object {} found", type);
      continue;
    }
    handlers.at(type)(ss, objs);
  }
  return objs;
}
} // namespace cg
