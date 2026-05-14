
module common_parsers;
import triangle;
import sphere;
import plane;
namespace cg
{
namespace parse_utils
{
void trim_line(std::string& s)
{
  std::size_t first = s.find_first_not_of(" \t\n\r\f\v");
  if (std::string::npos == first)
  {
    s.clear();
    return;
  }
  std::size_t last = s.find_last_not_of(" \t\n\r\f\v");
  s = s.substr(first, (last - first + 1));
}
} // namespace parse_utils

namespace parsers
{
namespace
{
constexpr float kMaterialEqEps = 1e-8f;

[[nodiscard]] bool same_color(color_rgb a, color_rgb b)
{
  return glm::length2(vec3{a - b}) <= kMaterialEqEps;
}

[[nodiscard]] bool same_material(const material& a, const material& b)
{
  return same_color(a.ambient, b.ambient) &&
         same_color(a.specular, b.specular) &&
         same_color(a.diffuse, b.diffuse) &&
         std::abs(a.shininess - b.shininess) <= kMaterialEqEps;
}
} // namespace

std::size_t add_inline_material(scene& s, const color_rgb& albedo)
{
  const material m = make_phong_material(albedo);
  const auto it = std::ranges::find_if(s.materials, [&](const material& c) {
    return same_material(c, m);
  });
  if (it != s.materials.cend())
  {
    return std::distance(std::begin(s.materials), it);
  }
  s.materials.emplace_back(m);
  return s.materials.size() - 1;
}

vec3 parse_vec3(std::istringstream& ss)
{
  vec3 v;
  if (!(ss >> v.x >> v.y >> v.z))
    return {0, 0, 0};
  return v;
}

color_rgb parse_color(std::istringstream& ss)
{
  const float imax_channel = 1.f / 255.99f;
  return color_rgb(parse_vec3(ss) * imax_channel);
}
void parse_sphere(std::istringstream& ss, scene& s)
{
  const vec3 center = parse_vec3(ss);

  float radius{};
  if (!(ss >> radius))
  {
    std::println(std::cerr, "bad sphere (missing radius)");
    return;
  }

  const color_rgb col = parse_color(ss);
  const auto material_id = add_inline_material(s, col);
  s.objects.push_back(sphere{radius, center, material_id});
  std::println("parsed sphere center=({}, {}, {}) radius={} material={}", center.x,
               center.y, center.z, radius, material_id);
}
void parse_triangle(std::istringstream& ss, scene& s)
{
  // triangle x1 y1 z1  x2 y2 z2  x3 y3 z3  R G B
  const vec3 v0 = parse_vec3(ss);
  const vec3 v1 = parse_vec3(ss);
  const vec3 v2 = parse_vec3(ss);

  const color_rgb col = parse_color(ss);
  const auto material_id = add_inline_material(s, col);

  s.objects.push_back(triangle{
      .p0 = v0,
      .p1 = v1,
      .p2 = v2,
      .material_id = material_id,
  });
  std::println("parsed triangle p0=({}, {}, {}) p1=({}, {}, {}) p2=({}, {}, {}) "
               "material={}",
               v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, material_id);
}
void parse_plane(std::istringstream& ss, scene& s)
{
  // plane px py pz nx ny nz
  const vec3 p = parse_vec3(ss);
  const vec3 n = parse_vec3(ss);

  std::size_t material_id{};
  if (!(ss >> material_id))
  {
    std::println(std::cerr, "bad plane (missing material id)");
    return;
  }
  if (material_id >= s.materials.size())
  {
    std::println(std::cerr, "bad plane (material id {} out of range [0, {}))",
                 material_id, s.materials.size());
    return;
  }

  s.objects.push_back(
      plane{.normal = n, .point = p, .material_id = material_id});
  std::println("parsed plane point=({}, {}, {}) normal=({}, {}, {}) material={}",
               p.x, p.y, p.z, n.x, n.y, n.z, material_id);
}

} // namespace parsers
} // namespace cg
