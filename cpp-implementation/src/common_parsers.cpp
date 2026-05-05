
module common_parsers;
import triangle;
import sphere;
namespace cg {
namespace parse_utils {
void trim_line(std::string &s) {
  std::size_t first = s.find_first_not_of(" \t\n\r\f\v");
  if (std::string::npos == first) {
    s.clear();
    return;
  }
  std::size_t last = s.find_last_not_of(" \t\n\r\f\v");
  s = s.substr(first, (last - first + 1));
}
} // namespace parse_utils

namespace parsers {
std::size_t add_inline_material(scene &s, const color_rgb &albedo) {
  s.materials.emplace_back(albedo);
  return s.materials.size() - 1;
}

vec3 parse_vec3(std::istringstream &ss) {
  vec3 v;
  if (!(ss >> v.x >> v.y >> v.z))
    return {0, 0, 0};
  return v;
}

color_rgb parse_color(std::istringstream &ss) {
  const float imax_channel = 1.f / 255.99f;
  return color_rgb(parse_vec3(ss) * imax_channel);
}
void parse_sphere(std::istringstream &ss, scene &s) {
  const vec3 center = parse_vec3(ss);

  float radius{};
  if (!(ss >> radius)) {
    std::println(std::cerr, "bad sphere (missing radius)");
    return;
  }

  const color_rgb col = parse_color(ss);

  const auto material_id = add_inline_material(s, col);
  s.objects.push_back(std::make_unique<sphere>(radius, center, col, material_id));
}
void parse_triangle(std::istringstream &ss, scene &s) {
  // triangle x1 y1 z1  x2 y2 z2  x3 y3 z3  R G B
  const vec3 v0 = parse_vec3(ss);
  const vec3 v1 = parse_vec3(ss);
  const vec3 v2 = parse_vec3(ss);

  const color_rgb col = parse_color(ss);
  const auto material_id = add_inline_material(s, col);

  s.objects.push_back(std::make_unique<triangle>(v0, v1, v2, col, material_id));
}

} // namespace parsers
} // namespace cg
