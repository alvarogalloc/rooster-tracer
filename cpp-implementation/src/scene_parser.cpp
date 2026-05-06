module scene_parser;
import object3d;
import triangle;
import sphere;
import vec3;
import color_rgb;
import obj_parser;
import common_parsers;
import light;
import directional_light;

namespace cg
{

namespace parsers
{

void parse_dir_light(std::istringstream& ss, scene& s)
{
  // of the form dir_light 0 -1 0 0 255 0 1.0
  //<dir> <color> <intensity>
  auto dir = parse_vec3(ss);
  auto color = parse_color(ss);
  float intensity;
  ss >> intensity;
  s.lights.push_back(directional_light{dir, color, intensity});
}
void parse_material(std::istringstream& ss, scene& s)
{
  // of the token "mat"
  //  albedo lambert color: just a color
  // e.g. mat 255 0 0

  auto color = parse_color(ss);
  s.materials.emplace_back(color);
}
} // namespace parsers
static const std::unordered_map<std::string,
                                void (*)(std::istringstream&, scene&)>
    handlers{
        {"sphere", &parsers::parse_sphere},
        {"triangle", &parsers::parse_triangle},
        {"obj", &parsers::parse_obj_file},
        {"mat", &parsers::parse_material},
        {"dir_light", &parsers::parse_dir_light},
    };
scene parse_scene(const std::string& filepath)
{

  constexpr static auto filetoken = "ROOSTERSCENEV1";
  std::ifstream f{filepath};
  if (not f.is_open())
  {
    throw std::runtime_error{
        std::format("scene file ({}) not found", filepath)};
  }
  std::string line;
  std::getline(f, line);
  if (not line.starts_with(filetoken))
  {
    std::println(std::cerr, "no file token {} found", filetoken);
  }
  scene s;
  while (std::getline(f, line))
  {
    parse_utils::trim_line(line);
    if (parse_utils::should_skip_line(line))
      continue;
    std::istringstream ss(line);
    std::string type;
    ss >> type;
    if (!handlers.count(type))
    {
      std::println(std::cerr, "unknown type of object {} found", type);
      continue;
    }
    handlers.at(type)(ss, s);
  }
  return s;
}
} // namespace cg
