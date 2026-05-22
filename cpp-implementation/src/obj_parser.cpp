module obj_parser;
import std;
import glm;
import common_parsers;
import mesh3d;
import bvh;
import triangle;
import color_rgb;
import material;
import texture;

namespace cg::parsers
{
namespace
{
constexpr int kMissingIndex = -1;
constexpr float kNormalEpsLen2 = 1e-12f;

struct face_vertex_ref
{
    int vertex_idx{kMissingIndex};
    int uv_idx{kMissingIndex};
    int normal_idx{kMissingIndex};
};

using face_polygon = std::vector<face_vertex_ref>;
struct face_info
{
    face_polygon vertices;
    std::size_t material_id;
};

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

[[nodiscard]] vec2 parse_vec2(std::istringstream& ss)
{
    vec2 v;
    if (!(ss >> v.x >> v.y))
        return {0.f, 0.f};
    return v;
}

[[nodiscard]] std::optional<vec3> parse_vec3_optional(std::istringstream& ss)
{
    vec3 v;
    if (!(ss >> v.x >> v.y >> v.z))
        return std::nullopt;
    return v;
}

[[nodiscard]] std::optional<float> parse_float_optional(std::istringstream& ss)
{
    float v = 0.f;
    if (!(ss >> v))
        return std::nullopt;
    return v;
}

[[nodiscard]] face_vertex_ref parse_face_token(std::string_view token,
                                               std::size_t vertex_count,
                                               std::size_t uv_count,
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
        throw std::runtime_error{std::format(
            "obj parser: vertex index {} out of range", raw_vertex)};
    }

    int uv_idx = kMissingIndex;
    if (first_slash != std::string_view::npos)
    {
        const auto uv_part =
            token.substr(first_slash + 1, second_slash - first_slash - 1);
        if (!uv_part.empty())
        {
            const int raw_uv = std::stoi(std::string{uv_part});
            uv_idx = obj_to_zero_based(raw_uv, uv_count);
            if (uv_idx < 0 || uv_idx >= static_cast<int>(uv_count))
            {
                throw std::runtime_error{std::format(
                    "obj parser: uv index {} out of range", raw_uv)};
            }
        }
    }

    int normal_idx = kMissingIndex;
    if (second_slash != std::string_view::npos &&
        second_slash + 1 < token.size())
    {
        const auto normal_part = token.substr(second_slash + 1);
        const int raw_normal = std::stoi(std::string{normal_part});
        normal_idx = obj_to_zero_based(raw_normal, normal_count);
        if (normal_idx < 0 || normal_idx >= static_cast<int>(normal_count))
        {
            throw std::runtime_error{std::format(
                "obj parser: normal index {} out of range", raw_normal)};
        }
    }

    return face_vertex_ref{
        .vertex_idx = vertex_idx, .uv_idx = uv_idx, .normal_idx = normal_idx};
}

void parse_face(std::istringstream& ss, std::size_t vertex_count,
                    std::size_t uv_count, std::size_t normal_count,
                    std::size_t material_id, std::vector<face_info>& faces)
{
    face_polygon refs;
    std::string token;
    while (ss >> token)
    {
        refs.push_back(
                parse_face_token(token, vertex_count, uv_count, normal_count));
    }
    if (refs.size() >= 3)
        faces.push_back(face_info{.vertices = std::move(refs),
                                      .material_id = material_id});
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

[[nodiscard]] std::string resolve_mtl_path(std::string_view mtl_path,
                                           std::string_view obj_path)
{
    const std::filesystem::path path{mtl_path};
    if (path.is_absolute())
        return path.string();
    const std::filesystem::path base =
        std::filesystem::path(obj_path).parent_path();
    return (base / path).lexically_normal().string();
}

struct mtl_material
{
    std::string name;
    std::optional<vec3> ambient;
    std::optional<vec3> diffuse;
    std::optional<vec3> specular;
    std::optional<float> shininess;
    std::optional<std::string> diffuse_map;
    std::optional<std::string> normal_map;
};

[[nodiscard]] std::string resolve_asset_path(std::string_view asset_path,
                                             std::string_view base_path)
{
    const std::filesystem::path path{asset_path};
    if (path.is_absolute())
        return path.string();
    const std::filesystem::path base =
        std::filesystem::path(base_path).parent_path();
    return (base / path).lexically_normal().string();
}

[[nodiscard]] std::size_t load_texture_id(
    scene& s, std::unordered_map<std::string, std::size_t>& texture_ids,
    const std::string& path)
{
    if (const auto it = texture_ids.find(path); it != texture_ids.end())
        return it->second;
    s.textures.emplace_back(load_texture(path));
    const std::size_t id = s.textures.size() - 1;
    texture_ids.emplace(path, id);
    std::println("loaded texture id={} path={}", id, path);
    return id;
}

[[nodiscard]] material to_material(
    const mtl_material& src,
    const std::optional<std::size_t>& texture_id = std::nullopt,
    const std::optional<std::size_t>& normal_map_id = std::nullopt)
{
    const color_rgb diffuse =
        src.diffuse ? color_rgb{*src.diffuse} : color_rgb{1.f, 1.f, 1.f};
    const color_rgb ambient = src.ambient
                                  ? color_rgb{*src.ambient}
                                  : color_rgb{diffuse * kDefaultAmbientFactor};
    const color_rgb specular =
        src.specular ? color_rgb{*src.specular} : color_rgb{1.f, 1.f, 1.f};
    const float shininess = src.shininess.value_or(kDefaultShininess);
    return material{
        .ambient = ambient,
        .specular = specular,
        .diffuse = diffuse,
        .shininess = shininess,
        .texture_id = texture_id,
        .normal_map_id = normal_map_id,
    };
}

void register_mtl_material(
    scene& s, std::unordered_map<std::string, std::size_t>& ids,
    std::unordered_map<std::string, std::size_t>& texture_ids,
    const std::string& mtl_path, const mtl_material& src)
{
    if (src.name.empty())
        return;
    if (ids.contains(src.name))
        return;
    std::optional<std::size_t> texture_id{};
    if (src.diffuse_map)
    {
        const std::string path =
            resolve_asset_path(*src.diffuse_map, mtl_path);
        texture_id = load_texture_id(s, texture_ids, path);
    }
    std::optional<std::size_t> normal_map_id{};
    if (src.normal_map)
    {
        const std::string path =
            resolve_asset_path(*src.normal_map, mtl_path);
        normal_map_id = load_texture_id(s, texture_ids, path);
    }
    s.materials.emplace_back(to_material(src, texture_id, normal_map_id));
    const std::size_t id = s.materials.size() - 1;
    ids.emplace(src.name, id);
    const auto& m = s.materials.back();
    const std::string tex_label =
        m.texture_id ? std::to_string(*m.texture_id) : "none";
    const std::string norm_label =
        m.normal_map_id ? std::to_string(*m.normal_map_id) : "none";
    std::println("parsed mtl material name={} id={} Ka=({}, {}, {}) "
                 "Kd=({}, {}, {}) Ks=({}, {}, {}) Ns={} tex_id={} norm_id={}",
                 src.name, id, m.ambient.x, m.ambient.y, m.ambient.z,
                 m.diffuse.x, m.diffuse.y, m.diffuse.z, m.specular.x,
                 m.specular.y, m.specular.z, m.shininess, tex_label, norm_label);
}

void parse_mtl_file(const std::string& filename, scene& s,
                    std::unordered_map<std::string, std::size_t>& ids,
                    std::unordered_map<std::string, std::size_t>& texture_ids)
{
    std::ifstream f{filename};
    if (!f.is_open())
    {
        std::println(std::cerr, "obj parser: mtl file not found ({})",
                     filename);
        return;
    }
    std::string line;
    mtl_material current{};
    auto flush = [&]() {
        register_mtl_material(s, ids, texture_ids, filename, current);
    };
    while (std::getline(f, line))
    {
        parse_utils::trim_line(line);
        if (parse_utils::should_skip_line(line))
            continue;
        std::istringstream ss(line);
        std::string type;
        ss >> type;
        if (type == "newmtl")
        {
            flush();
            ss >> current.name;
            current.ambient.reset();
            current.diffuse.reset();
            current.specular.reset();
            current.shininess.reset();
            current.diffuse_map.reset();
            continue;
        }
        if (type == "Ka")
        {
            current.ambient = parse_vec3_optional(ss);
            continue;
        }
        if (type == "Kd")
        {
            current.diffuse = parse_vec3_optional(ss);
            continue;
        }
        if (type == "Ks")
        {
            current.specular = parse_vec3_optional(ss);
            continue;
        }
        if (type == "Ns")
        {
            current.shininess = parse_float_optional(ss);
            continue;
        }
        if (type == "map_Kd")
        {
            std::string map_path;
            ss >> map_path;
            if (!map_path.empty())
                current.diffuse_map = map_path;
            continue;
        }
        if (type == "map_Bump" || type == "map_bump" || type == "bump")
        {
            std::string map_path;
            ss >> map_path;
            // Handle optional bump multiplier e.g. -bm 1.000000
            if (map_path == "-bm")
            {
                float multiplier;
                ss >> multiplier;
                ss >> map_path;
            }
            if (!map_path.empty())
                current.normal_map = map_path;
            continue;
        }
    }
    flush();
}

[[nodiscard]] std::size_t resolve_material_id(
    std::string_view name,
    const std::unordered_map<std::string, std::size_t>& ids,
    std::size_t fallback_id)
{
    const auto it = ids.find(std::string{name});
    if (it == ids.end())
        return fallback_id;
    return it->second;
}
} // namespace

void parse_obj_file_contents(const std::string& filename, scene& s, vec3 origin,
                             std::size_t material_id)
{
    std::println("loading obj: {}", filename);
    std::ifstream f{filename};
    if (!f.is_open())
    {
        throw std::runtime_error{
            std::format("obj file ({}) not found", filename)};
    }
    if (material_id >= s.materials.size())
    {
        throw std::runtime_error{"obj parser: material out of bounds"};
    }

    std::vector<vec3> obj_positions;
    std::vector<vec2> obj_uvs;
    std::vector<vec3> obj_normals;
    std::vector<face_info> faces;
    std::unordered_map<std::string, std::size_t> mtl_ids;
    std::unordered_map<std::string, std::size_t> texture_ids;
    std::size_t current_material_id = material_id;
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
            obj_positions.push_back(parse_vec3(ss) + origin);
            continue;
        }
        if (type == "vt")
        {
            obj_uvs.push_back(parse_vec2(ss));
            continue;
        }
        if (type == "vn")
        {
            obj_normals.push_back(safe_normalize(parse_vec3(ss)));
            continue;
        }
        if (type == "mtllib")
        {
            std::string mtl_token;
            while (ss >> mtl_token)
            {
                parse_mtl_file(resolve_mtl_path(mtl_token, filename), s,
                               mtl_ids, texture_ids);
            }
            continue;
        }
        if (type == "usemtl")
        {
            std::string material_name;
            ss >> material_name;
            current_material_id =
                resolve_material_id(material_name, mtl_ids, material_id);
            continue;
        }
        if (type == "f")
        {
            parse_face(ss, obj_positions.size(), obj_uvs.size(),
                       obj_normals.size(), current_material_id, faces);
        }
    }

    std::vector<vec3> generated_normals(obj_positions.size(),
                                        vec3{0.f, 0.f, 0.f});
    for (const auto& face : faces)
    {
        const auto& verts = face.vertices;
        for (std::size_t i = 1; i + 1 < verts.size(); ++i)
        {
            const vec3& a = obj_positions.at(verts[0].vertex_idx);
            const vec3& b = obj_positions.at(verts[i].vertex_idx);
            const vec3& c = obj_positions.at(verts[i + 1].vertex_idx);
            const vec3 fn = face_normal(a, b, c);
            generated_normals[verts[0].vertex_idx] += fn;
            generated_normals[verts[i].vertex_idx] += fn;
            generated_normals[verts[i + 1].vertex_idx] += fn;
        }
    }
    std::transform(generated_normals.begin(), generated_normals.end(),
                   generated_normals.begin(), safe_normalize);

    const auto first_tri = s.mesh_triangles.size();
    for (const auto& face : faces)
    {
        const auto& verts = face.vertices;
        for (std::size_t i = 1; i + 1 < verts.size(); ++i)
        {
            const auto& f0 = verts[0];
            const auto& f1 = verts[i];
            const auto& f2 = verts[i + 1];

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
            auto pick_uv = [&](const face_vertex_ref& ref) {
                if (ref.uv_idx >= 0)
                    return obj_uvs.at(ref.uv_idx);
                return vec2{0.f, 0.f};
            };

            const auto base_vertex = s.vertices.size();
            s.vertices.push_back(vertex{
                .p = p0,
                .n = pick_normal(f0),
                .uv = pick_uv(f0),
                .has_normal = true,
                .has_uv = f0.uv_idx >= 0,
            });
            s.vertices.push_back(vertex{
                .p = p1,
                .n = pick_normal(f1),
                .uv = pick_uv(f1),
                .has_normal = true,
                .has_uv = f1.uv_idx >= 0,
            });
            s.vertices.push_back(vertex{
                .p = p2,
                .n = pick_normal(f2),
                .uv = pick_uv(f2),
                .has_normal = true,
                .has_uv = f2.uv_idx >= 0,
            });

            s.mesh_triangles.push_back(triangle{
                .vertex_start = base_vertex,
                .material_id = face.material_id,
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
            throw std::runtime_error{"obj parser: material index provided but "
                                     "no materials are defined"};
        }
        if (material_id >= s.materials.size())
        {
            throw std::runtime_error{std::format(
                "obj parser: material index {} out of range [0, {})",
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
