export module common_parsers;
import vec3;
import std;
import scene;
import color_rgb;
import object3d;
export namespace cg {
namespace parse_utils {
void trim_line(std::string &s);
inline bool should_skip_line(std::string_view line) {
  return line.starts_with('#') or line.starts_with(' ') or line.size() == 0;
}

} // namespace parse_utils

namespace parsers {
vec3 parse_vec3(std::istringstream &ss);
color_rgb parse_color(std::istringstream &ss);
void parse_sphere(std::istringstream &ss, scene &);
void parse_triangle(std::istringstream &ss, scene &);
} // namespace parsers
} // namespace cg
