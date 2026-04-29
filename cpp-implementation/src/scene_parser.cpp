module scene_parser;

import triangle;
import sphere;
import vec3;
import color_rgb;
import object3d;
namespace cg {

namespace parsers {
vec3 parse_vec3(std::istringstream& ss) {
    float x{}, y{}, z{};
    if (!(ss >> x >> y >> z)) return {0,0,0};
    return vec3{x, y, z};
  }

  color_rgb parse_color(std::istringstream& ss) {
    const float imax_channel= 1.f/255.99f;
    return color_rgb(parse_vec3(ss) * imax_channel);
  }
  void parse_sphere(std::istringstream &ss,std::vector<std::unique_ptr<object3d>>&objects ){
  const vec3 center = parse_vec3(ss);

  float radius{};
  if (!(ss >> radius)) {
    std::println(std::cerr, "bad sphere (missing radius)");
    return;
  }

  const color_rgb col = parse_color(ss);

  // Assumes: sphere(vec3 center, float radius, color_rgb color) and sphere : object3d
  objects.push_back(std::make_unique<sphere>(radius, center, col));
  }
   void parse_triangle(std::istringstream &ss,std::vector<std::unique_ptr<object3d>>&objects ){
 // triangle x1 y1 z1  x2 y2 z2  x3 y3 z3  R G B
  const vec3 v0 = parse_vec3(ss);
  const vec3 v1 = parse_vec3(ss);
  const vec3 v2 = parse_vec3(ss);

  const color_rgb col = parse_color(ss);

  objects.push_back(std::make_unique<triangle>(v0, v1, v2, col));
  }
}

 scene parse_scene(const std::string& filepath) {


  std::unordered_map<
    std::string,
    void(*)(std::istringstream&, std::vector<std::unique_ptr<object3d>>&)
  > handlers {
        {"sphere", &parsers::parse_sphere},
        {"triangle", &  parsers::parse_triangle},
};
constexpr static auto filetoken="ROOSTERSCENEV1";
  std::ifstream f{filepath};
  std::string line;
  std::getline(f, line);
  if (not line.starts_with(filetoken)) {
    std::println(std::cerr, "no file token {} found", filetoken);
  }
  scene s;
  while (std::getline(f, line)) 
   {
    std::istringstream ss(line);
    std::string type;
    ss >> type;
    if(!handlers.count(type)) {
      std::println(std::cerr, "unknown type of object {} found", type);
      continue;
    }
    handlers[type](ss,s.objects);
   }
  return s;
 }
}