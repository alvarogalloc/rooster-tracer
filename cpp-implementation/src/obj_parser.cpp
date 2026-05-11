module obj_parser;
import vec3;
import common_parsers;
import mesh3d;
import aabb;
import bvh;
import triangle;
import color_rgb;

/*
 * obj parser:
 * - for now only vertices and faces (non quads yet)
 *
 *   the key is the token_map: it handles -in O(1)- the correct function for the
 * current line
 */

namespace cg::parsers
{
void parse_vertex(std::istringstream& ss, scene&, std::vector<vec3>& vertices,
                  vec3 origin, std::size_t)
{
  vertices.push_back(parse_vec3(ss) - origin);
}

void parse_face(std::istringstream& line_stream, scene& s,
                std::vector<vec3>& vertices, vec3, std::size_t material_id)
{
  std::vector<int> indices;
  std::string token;

  while (line_stream >> token)
  {
    int idx =
        std::stoi(token.substr(0, token.find('/'))) - 1; // OBJ is 1-indexed
    if (idx < 0 || idx >= static_cast<int>(vertices.size()))
      return;
    indices.push_back(idx);
  }

  // Need at least 3 vertices
  if (indices.size() < 3)
    return;

  // Fan triangulation
  for (std::size_t i = 1; i + 1 < indices.size(); ++i)
  {
    s.mesh_triangles.push_back(cg::triangle(vertices.at(indices[0]),
                                            vertices.at(indices[i]),
                                            vertices.at(indices[i + 1])));
  }
}

static const std::unordered_map<std::string,
                                void (*)(std::istringstream&, scene&,
                                         std::vector<vec3>&, vec3, std::size_t)>
    token_map{
        {"v", &parse_vertex},
        {"f", &parse_face},
    };
void parse_obj_file_contents(const std::string& filename, scene& s, vec3 origin,
                             std::size_t material_id)
{
  std::println("loading obj: {}", filename);
  std::ifstream f{filename};
  if (not f.is_open())
  {
    throw std::runtime_error{std::format("obj file ({}) not found", filename)};
  }
  if (material_id >= s.materials.size())
    throw std::runtime_error{"material out of bounds"};
  std::string line;
  std::vector<vec3> vertices;
  auto current_obj_number = s.mesh_triangles.size();
  while (std::getline(f, line))
  {
    parse_utils::trim_line(line);
    if (parse_utils::should_skip_line(line))
    {
      continue;
    }
    std::istringstream ss(line);
    std::string type;
    ss >> type;
    if (not token_map.count(type))
    {
      continue;
    }
    token_map.at(type)(ss, s, vertices, origin, material_id);
  }
  std::println("done loading model (vertex count: {}, face count: {})",
               vertices.size(), s.mesh_triangles.size() - current_obj_number);
  mesh3d& new_mesh = std::get<mesh3d>(s.objects.emplace_back(
      mesh3d{current_obj_number, s.mesh_triangles.size() - current_obj_number,
             material_id}));
  build_bvh(new_mesh.blas, std::span{s.mesh_triangles}.subspan(
                               new_mesh.vertex_start, new_mesh.vertex_count));
}

void parse_obj_file(std::istringstream& ss, scene& s)
{
  std::string filename;
  ss >> filename;

  vec3 origin{0, 0, 0};
  float ox{}, oy{}, oz{};
  if (ss >> ox >> oy >> oz)
  {
    origin = vec3{ox, oy, oz};
  }
  else
  {
    ss.clear();
  }

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
