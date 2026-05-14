module scene_parser;
import triangle;
import sphere;
import glm;
import color_rgb;
import obj_parser;
import common_parsers;
import material;
import light;
import directional_light;
import point_light;

namespace cg
{

namespace parsers
{

void parse_dir_light(std::istringstream& ss, scene& s)
{
  // of the form dir_light 0 -1 0 0 255 0 1.0
  //<dir> <color> <intensity>
  const auto dir = parse_vec3(ss);
  const auto color = parse_color(ss);
  float intensity = 1.f;
  if (!(ss >> intensity))
  {
    std::println(std::cerr,
                 "bad dir_light (missing intensity), using default 1.0");
    ss.clear();
  }
  s.lights.push_back(directional_light{dir, color, intensity});
  std::println("parsed dir_light dir=({}, {}, {}) intensity={}", dir.x, dir.y,
               dir.z, intensity);
}
void parse_material(std::istringstream& ss, scene& s)
{
  // of the token "mat"
  //  albedo lambert color: just a color
  // e.g. mat 255 0 0

  const auto color = parse_color(ss);
  s.materials.emplace_back(make_phong_material(color));
  std::println("parsed material id={} rgb=({}, {}, {})", s.materials.size() - 1,
               color.x, color.y, color.z);
}

void parse_point_light(std::istringstream& ss, scene& s)
{
  // of the form point_light x y z r g b intensity
  const auto pos = parse_vec3(ss);
  const auto color = parse_color(ss);
  float intensity = 1.f;
  if (!(ss >> intensity))
  {
    std::println(std::cerr,
                 "bad point_light (missing intensity), using default 1.0");
    ss.clear();
  }
  s.lights.push_back(point_light{pos, color, intensity});
  std::println("parsed point_light pos=({}, {}, {}) intensity={}", pos.x, pos.y,
               pos.z, intensity);
}

void parse_viewport(std::istringstream& ss, scene& s)
{
  int width = 0;
  int height = 0;
  if (!(ss >> width >> height))
  {
    std::println(std::cerr, "bad viewport (expected: viewport width height)");
    return;
  }
  s.camera_data.width = width;
  s.camera_data.height = height;
  std::println("parsed viewport width={} height={}", width, height);
}

void parse_camera(std::istringstream& ss, scene& s)
{
  const vec3 pos = parse_vec3(ss);
  const vec3 look_at = parse_vec3(ss);
  const vec3 up = parse_vec3(ss);
  float near_plane = s.camera_data.near;
  float far_plane = s.camera_data.far;
  if (!(ss >> near_plane >> far_plane))
  {
    std::println(std::cerr,
                 "bad camera (expected near/far), using defaults {} and {}",
                 s.camera_data.near, s.camera_data.far);
    ss.clear();
  }

  s.camera_data.pos = pos;
  s.camera_data.lookAt = look_at;
  s.camera_data.up = up;
  s.camera_data.near = near_plane;
  s.camera_data.far = far_plane;

  std::println("parsed camera pos=({}, {}, {}) look_at=({}, {}, {}) "
               "up=({}, {}, {}) near={} far={}",
               pos.x, pos.y, pos.z, look_at.x, look_at.y, look_at.z, up.x, up.y,
               up.z, near_plane, far_plane);
}

void parse_fov(std::istringstream& ss, scene& s)
{
  float degrees = 0.f;
  if (!(ss >> degrees))
  {
    std::println(std::cerr, "bad fov (expected degrees)");
    return;
  }
  s.camera_data.fov = glm::radians(degrees);
  std::println("parsed fov degrees={} radians={}", degrees, s.camera_data.fov);
}

void parse_background(std::istringstream& ss, scene& s)
{
  const color_rgb bg = parse_color(ss);
  s.background_color = bg;
  std::println("parsed background rgb=({}, {}, {})", bg.x, bg.y, bg.z);
}

void parse_max_depth(std::istringstream& ss, scene& s)
{
  int depth = 0;
  if (!(ss >> depth))
  {
    std::println(std::cerr, "bad max_depth (expected positive integer)");
    return;
  }
  s.max_depth = static_cast<u32>(std::max(1, depth));
  std::println("parsed max_depth={}", s.max_depth);
}
} // namespace parsers
static const std::unordered_map<std::string,
                                void (*)(std::istringstream&, scene&)>
    handlers{
        {"sphere", &parsers::parse_sphere},
        {"triangle", &parsers::parse_triangle},
        {"obj", &parsers::parse_obj_file},
        {"plane", &parsers::parse_plane},
        {"mat", &parsers::parse_material},
        {"dir_light", &parsers::parse_dir_light},
        {"point_light", &parsers::parse_point_light},
        {"viewport", &parsers::parse_viewport},
        {"camera", &parsers::parse_camera},
        {"fov", &parsers::parse_fov},
        {"background", &parsers::parse_background},
        {"max_depth", &parsers::parse_max_depth},
    };
scene parse_scene(const std::string& filepath)
{

  constexpr static auto filetoken = "ROOSTERSCENEV1";
  std::println("parsing scene: {}", filepath);
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
    throw std::runtime_error{
        std::format("invalid scene token, expected {}", filetoken)};
  }
  scene s;
  const auto parent = std::filesystem::path(filepath).parent_path();
  s.source_dir = parent.empty() ? "." : parent.string();
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
  std::println("scene parsed: objects={} materials={} lights={} mesh_tris={}",
               s.objects.size(), s.materials.size(), s.lights.size(),
               s.mesh_triangles.size());
  return s;
}
} // namespace cg
