module obj_parser;
import vec3;
import common_parsers;
import triangle;
import color_rgb;

/*
 * obj parser:
 * - for now only vertices and faces (non quads yet)
 *
 *   the key is the token_map: it handles -in O(1)- the correct function for the
 * current line
 */

namespace cg::parsers {
void parse_vertex(std::istringstream &ss, scene::object_collection &,
                  std::vector<vec3> &vertices) {
  vertices.push_back(parse_vec3(ss) - vec3{0, 0, 0});
}

void parse_face(std::istringstream &line_stream,
                scene::object_collection &objects,
                std::vector<vec3> &vertices) {
  std::array<int, 3> indices{};
  for (auto &idx : indices) {
    std::string token;
    line_stream >> token;
    // OBJ supports a lot of variant, for now we only take the index of the
    // vertex
    idx = std::stoi(token.substr(0, token.find('/'))) - 1; // OBJ is 1-indexed
    if (idx <= 0 or idx >= vertices.size())
      return;
  }

  static float channel{0.8f};
  objects.push_back(std::make_unique<cg::triangle>(
      vertices.at(indices[0]), vertices.at(indices[1]), vertices.at(indices[2]),
      cg::color_rgb{channel, channel - 0.2f, channel / 4}));
  channel += 0.05f;
  if (channel >= 1.f)
    channel = 0.8f;
}

static const std::unordered_map<
    std::string, void (*)(std::istringstream &, scene::object_collection &,
                          std::vector<vec3> &)>
    token_map{
        {"v", &parse_vertex},
        {"f", &parse_face},
    };
void parse_obj_file_contents(const std::string &filename,
                             scene::object_collection &objs) {
  std::println("loading obj: {}", filename);
  std::ifstream f{filename};
  if (not f.is_open()) {
    throw std::runtime_error{std::format("obj file ({}) not found", filename)};
  }
  std::string line;
  int line_i = 0;
  std::vector<vec3> vertices;
  auto current_obj_number = objs.size();
  while (std::getline(f, line)) {
    line_i++;
    parse_utils::trim_line(line);
    std::println(std::cerr, "line {}: {}", line_i, line);
    if (parse_utils::should_skip_line(line)) {
      continue;
    }
    std::istringstream ss(line);
    std::string type;
    ss >> type;
    if (not token_map.count(type)) {
      std::println(std::cerr, "unexpected token at line {}: \"{}\"", line_i,
                   type);
      continue;
    }
    token_map.at(type)(ss, objs, vertices);
  }
  std::println("done loading model (vertex count: {}, face count: {})",
              vertices.size(), objs.size() - current_obj_number);
}

void parse_obj_file(std::istringstream &ss, scene::object_collection &objects) {
  std::string filename;
  ss >> filename;
  parse_obj_file_contents(filename, objects);
}
} // namespace cg::parsers
