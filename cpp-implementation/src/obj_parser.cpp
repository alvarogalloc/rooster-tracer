module obj_parser;
import std;
import glm;
import common_parsers;
import mesh3d;
import bvh;
import triangle;
import color_rgb;

namespace cg::parsers
{
namespace
{
constexpr int kMissingIndex = -1;
constexpr float kNormalEpsLen2 = 1e-12f;

struct face_vertex_ref
{
  int vertex_idx{kMissingIndex};
  int normal_idx{kMissingIndex};
};

using face_polygon = std::vector<face_vertex_ref>;

[[nodiscard]] int obj_to_zero_based(int raw_index, std::size_t count)
{
  if (raw_index > 0)
    return raw_index - 1;
  if (raw_index < 0)
    return static_cast<int>(count) + raw_index;
  return kMissingIndex;
}

[[nodiscard]] vec3 safe_normalize(vec3 v)
{
  const float len_sq = glm::dot(v, v);
  if (len_sq <= kNormalEpsLen2)
    return vec3{0.f, 0.f, 0.f};
  return v * glm::inversesqrt(len_sq);
}

[[nodiscard]] face_vertex_ref parse_face_token(std::string_view token,
                                               std::size_t vertex_count,
                                               std::size_t normal_count)
{
  const auto first_slash = token.find('/');
  const auto second_slash = first_slash == std::string_view::npos
                                ? std::string_view::npos
                                : token.find('/', first_slash + 1);

  const auto vertex_part = token.substr(0, first_slash);
  if (vertex_part.empty())
    throw std::runtime_error{"obj parser: malformed face vertex index"};

  const int raw_vertex = std::stoi(std::string{vertex_part});
  const int vertex_idx = obj_to_zero_based(raw_vertex, vertex_count);
  if (vertex_idx < 0 || vertex_idx >= static_cast<int>(vertex_count))
  {
    throw std::runtime_error{
        std::format("obj parser: vertex index {} out of range", raw_vertex)};
  }

  int normal_idx = kMissingIndex;
  if (second_slash != std::string_view::npos && second_slash + 1 < token.size())
  {
    const auto normal_part = token.substr(second_slash + 1);
    const int raw_normal = std::stoi(std::string{normal_part});
    normal_idx = obj_to_zero_based(raw_normal, normal_count);
    if (normal_idx < 0 || normal_idx >= static_cast<int>(normal_count))
    {
      throw std::runtime_error{
          std::format("obj parser: normal index {} out of range", raw_normal)};
    }
  }

  return face_vertex_ref{.vertex_idx = vertex_idx, .normal_idx = normal_idx};
}

void parse_face(std::istringstream& ss, std::size_t vertex_count,
                std::size_t normal_count, std::vector<face_polygon>& faces)
{
  face_polygon refs;
  std::string token;
  while (ss >> token)
  {
    refs.push_back(parse_face_token(token, vertex_count, normal_count));
  }
  if (refs.size() >= 3)
    faces.push_back(std::move(refs));
}

[[nodiscard]] vec3 face_normal(const vec3& a, const vec3& b, const vec3& c)
{
  return safe_normalize(glm::cross(b - a, c - a));
}

[[nodiscard]] std::string resolve_obj_path(std::string_view obj_path,
                                           std::string_view scene_source_dir)
{
  const std::filesystem::path path{obj_path};
  if (path.is_absolute())
    return path.string();
  return (std::filesystem::path(scene_source_dir) / path)
      .lexically_normal()
      .string();
}
} // namespace

void parse_obj_file_contents(const std::string& filename, scene& s, vec3 origin,
                             std::size_t material_id)
{
  std::println("loading obj: {}", filename);
  std::ifstream f{filename};
  if (!f.is_open())
  {
    throw std::runtime_error{std::format("obj file ({}) not found", filename)};
  }
  if (material_id >= s.materials.size())
  {
    throw std::runtime_error{"obj parser: material out of bounds"};
  }

  std::vector<vec3> obj_positions;
  std::vector<vec3> obj_normals;
  std::vector<face_polygon> faces;
  std::string line;
  while (std::getline(f, line))
  {
    parse_utils::trim_line(line);
    if (parse_utils::should_skip_line(line))
      continue;

    std::istringstream ss(line);
    std::string type;
    ss >> type;
    if (type == "v")
    {
      obj_positions.push_back(parse_vec3(ss) - origin);
      continue;
    }
    if (type == "vn")
    {
      obj_normals.push_back(safe_normalize(parse_vec3(ss)));
      continue;
    }
    if (type == "f")
    {
      parse_face(ss, obj_positions.size(), obj_normals.size(), faces);
    }
  }

  std::vector<vec3> generated_normals(obj_positions.size(),
                                      vec3{0.f, 0.f, 0.f});
  for (const auto& face : faces)
  {
    for (std::size_t i = 1; i + 1 < face.size(); ++i)
    {
      const vec3& a = obj_positions.at(face[0].vertex_idx);
      const vec3& b = obj_positions.at(face[i].vertex_idx);
      const vec3& c = obj_positions.at(face[i + 1].vertex_idx);
      const vec3 fn = face_normal(a, b, c);
      generated_normals[face[0].vertex_idx] += fn;
      generated_normals[face[i].vertex_idx] += fn;
      generated_normals[face[i + 1].vertex_idx] += fn;
    }
  }
  for (auto& n : generated_normals)
    n = safe_normalize(n);

  const auto first_tri = s.mesh_triangles.size();
  for (const auto& face : faces)
  {
    for (std::size_t i = 1; i + 1 < face.size(); ++i)
    {
      const auto& f0 = face[0];
      const auto& f1 = face[i];
      const auto& f2 = face[i + 1];

      const vec3& p0 = obj_positions.at(f0.vertex_idx);
      const vec3& p1 = obj_positions.at(f1.vertex_idx);
      const vec3& p2 = obj_positions.at(f2.vertex_idx);

      const vec3 fallback = face_normal(p0, p1, p2);
      auto pick_normal = [&](const face_vertex_ref& ref) {
        if (ref.normal_idx >= 0)
          return obj_normals.at(ref.normal_idx);
        const vec3 generated = generated_normals.at(ref.vertex_idx);
        if (glm::dot(generated, generated) > kNormalEpsLen2)
          return generated;
        return fallback;
      };

      const auto base_vertex = s.vertices.size();
      s.vertices.push_back(vertex{
          .p = p0,
          .n = pick_normal(f0),
          .has_normal = true,
      });
      s.vertices.push_back(vertex{
          .p = p1,
          .n = pick_normal(f1),
          .has_normal = true,
      });
      s.vertices.push_back(vertex{
          .p = p2,
          .n = pick_normal(f2),
          .has_normal = true,
      });

      s.mesh_triangles.push_back(triangle{
          .vertex_start = base_vertex,
          .material_id = material_id,
      });
    }
  }

  const auto tri_count = s.mesh_triangles.size() - first_tri;
  std::println("done loading model (vertex count: {}, face count: {})",
               obj_positions.size(), tri_count);

  mesh3d& new_mesh = std::get<mesh3d>(
      s.objects.emplace_back(mesh3d{first_tri, tri_count, material_id}));
  build_bvh(new_mesh.blas,
            std::span{s.mesh_triangles}.subspan(new_mesh.triangle_start,
                                                new_mesh.triangle_count),
            s.vertices);
}

void parse_obj_file(std::istringstream& ss, scene& s)
{
  std::string filename_token;
  ss >> filename_token;
  const std::string filename = resolve_obj_path(filename_token, s.source_dir);

  vec3 origin{0, 0, 0};
  float ox{}, oy{}, oz{};
  if (ss >> ox >> oy >> oz)
    origin = vec3{ox, oy, oz};
  else
    ss.clear();

  std::size_t material_id = 0;
  if (ss >> material_id)
  {
    if (s.materials.empty())
    {
      throw std::runtime_error{
          "obj parser: material index provided but no materials are defined"};
    }
    if (material_id >= s.materials.size())
    {
      throw std::runtime_error{
          std::format("obj parser: material index {} out of range [0, {})",
                      material_id, s.materials.size())};
    }
  }
  else
  {
    ss.clear();
  }

  parse_obj_file_contents(filename, s, origin, material_id);
}
} // namespace cg::parsers
