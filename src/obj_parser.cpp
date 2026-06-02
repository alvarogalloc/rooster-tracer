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
constexpr double kNormalEpsLen2 = 1e-12f;

struct face_vertex_ref
{
    int vertex_idx{kMissingIndex};
    int uv_idx{kMissingIndex};
    int normal_idx{kMissingIndex};
};

struct mtl_material
{
    std::string name;
    std::optional<vec3> ambient;
    std::optional<vec3> diffuse;
    std::optional<vec3> specular;
    std::optional<vec3> emissive;
    std::optional<double> shininess;
    std::optional<double> dissolve;
    std::optional<double> ior;
    std::optional<int> illum;
    std::optional<std::string> diffuse_map;
    std::optional<std::string> specular_map;
    std::optional<std::string> normal_map;
    std::optional<double> metalness;
    std::optional<double> roughness;
    std::optional<std::string> metallic_roughness_map;
};

struct mtl_state
{
    const std::string& filename;
    scene& s;
    std::unordered_map<std::string, std::size_t>& ids;
    std::unordered_map<std::string, std::size_t>& texture_ids;
    mtl_material current;
};

using mtl_handler_t = void (*)(std::istringstream&, mtl_state&);

struct obj_state
{
    const std::string& filename;
    scene& s;
    vec3 origin;
    std::size_t material_id;

    std::vector<vec3> obj_positions;
    std::vector<vec2> obj_uvs;
    std::vector<vec3> obj_normals;

    std::vector<int> face_vertex_indices;
    std::vector<int> face_uv_indices;
    std::vector<int> face_normal_indices;
    std::vector<std::size_t> face_offsets;
    std::vector<std::size_t> face_material_ids;

    std::unordered_map<std::string, std::size_t> mtl_ids;
    std::unordered_map<std::string, std::size_t> texture_ids;
    std::size_t current_material_id;
};

using obj_handler_t = void (*)(std::istringstream&, obj_state&);

// ---------------------------------------------------------
// MTL Helper Functions and Handlers
// ---------------------------------------------------------

[[nodiscard]] std::optional<vec3> parse_vec3_optional(std::istringstream& ss)
{
    vec3 v;
    if (!(ss >> v.x >> v.y >> v.z))
        return std::nullopt;
    return v;
}

[[nodiscard]] std::optional<double> parse_float_optional(std::istringstream& ss)
{
    double v = 0.;
    if (!(ss >> v))
        return std::nullopt;
    return v;
}

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
    const std::string& path, bool is_srgb = true)
{
    const std::string key = path + (is_srgb ? "@srgb" : "@linear");
    if (const auto it = texture_ids.find(key); it != texture_ids.end())
        return it->second;
    s.textures.emplace_back(load_texture(path, is_srgb));
    const std::size_t id = s.textures.size() - 1;
    texture_ids.emplace(key, id);
    std::println("loaded texture id={} path={} is_srgb={}", id, path, is_srgb);
    return id;
}

[[nodiscard]] material to_material(
    const mtl_material& src,
    const std::optional<std::size_t>& texture_id = std::nullopt,
    const std::optional<std::size_t>& normal_map_id = std::nullopt,
    const std::optional<std::size_t>& metallic_roughness_map_id = std::nullopt)
{
    const color_rgb diffuse =
        src.diffuse ? color_rgb{*src.diffuse} : color_rgb{0.8, 0.8, 0.8};

    color_rgb ambient;
    if (src.ambient)
    {
        const vec3 ka = *src.ambient;
        const bool is_white_ka = ka.x >= 0.99 && ka.y >= 0.99 && ka.z >= 0.99;
        ambient = is_white_ka ? color_rgb{diffuse * kDefaultAmbientFactor}
                              : color_rgb{ka * kDefaultAmbientFactor};
    }
    else
    {
        ambient = color_rgb{diffuse * kDefaultAmbientFactor};
    }

    const color_rgb specular =
        src.specular ? color_rgb{*src.specular} : color_rgb{0.5, 0.5, 0.5};

    const double shininess = src.shininess.value_or(kDefaultShininess);

    double transparency = 0.0;
    if (src.dissolve)
    {
        transparency = 1.0 - *src.dissolve;
    }

    double reflectivity = 0.0;
    if (src.illum)
    {
        const int model = *src.illum;

        // Illum 3, 4, 6, 7, and 9 explicitly call for ray-traced reflections.
        if (model == 3 || model == 4 || model == 6 || model == 7 || model == 9)
        {
            // Calculate standard relative luminance of the specular color
            // to determine how much of the environment it should reflect.
            reflectivity =
                0.2126 * specular.x + 0.7152 * specular.y + 0.0722 * specular.z;

            // Optional safety: If the artist set illum 3 but forgot Ks, give a
            // tiny fallback
            if (reflectivity <= 0.001)
                reflectivity = 0.1;
        }
        else
        {
            // Illum 1 or 2 means local shading ONLY. No ray-traced environment
            // bounces.
            reflectivity = 0.0;
        }
    }
    else if (src.specular)
    {
        double spec_intensity = glm::length(*src.specular);
        if (spec_intensity > 0.1)
        {
            reflectivity =
                glm::clamp(spec_intensity / std::sqrt(3.0), 0.0, 1.0);
        }
    }

    double ior = src.ior.value_or(1.0);

    double metalness = src.metalness.value_or(0.0);
    double roughness =
        src.roughness.value_or(std::sqrt(2.0 / (shininess + 2.0)));
    if (metallic_roughness_map_id)
    {
        metalness = 1.0;
        roughness = 1.0;
    }

    return material{
        .ambient = ambient,
        .specular = specular,
        .diffuse = diffuse,
        .shininess = shininess,
        .reflectivity = reflectivity,
        .transparency = transparency,
        .ior = ior,
        .texture_id = texture_id,
        .normal_map_id = normal_map_id,
        .metalness = metalness,
        .roughness = roughness,
        .metallic_roughness_map_id = metallic_roughness_map_id,
    };
}

void register_mtl_material(mtl_state& state)
{
    const mtl_material& src = state.current;
    if (src.name.empty() || state.ids.contains(src.name))
        return;
    std::optional<std::size_t> texture_id{};
    if (src.diffuse_map)
    {
        const std::string path =
            resolve_asset_path(*src.diffuse_map, state.filename);
        texture_id = load_texture_id(state.s, state.texture_ids, path, true);
    }
    std::optional<std::size_t> normal_map_id{};
    if (src.normal_map)
    {
        const std::string path =
            resolve_asset_path(*src.normal_map, state.filename);
        normal_map_id =
            load_texture_id(state.s, state.texture_ids, path, false);
    }
    std::optional<std::size_t> metallic_roughness_map_id{};
    if (src.metallic_roughness_map)
    {
        const std::string path =
            resolve_asset_path(*src.metallic_roughness_map, state.filename);
        metallic_roughness_map_id =
            load_texture_id(state.s, state.texture_ids, path, false);
    }
    else
    {
        // 1. Check if <material_name>_metallicRoughness.png exists in the same
        // directory as the .mtl file
        std::filesystem::path mtl_dir =
            std::filesystem::path(state.filename).parent_path();
        std::filesystem::path mr_file =
            mtl_dir / (src.name + "_metallicRoughness.png");
        if (std::filesystem::exists(mr_file))
        {
            metallic_roughness_map_id = load_texture_id(
                state.s, state.texture_ids, mr_file.string(), false);
        }
        // 2. Try replacing keywords in the diffuse texture filepath if it
        // exists
        else if (src.diffuse_map)
        {
            std::string diff_path = *src.diffuse_map;
            const std::vector<std::pair<std::string, std::string>> patterns = {
                {"_baseColor", "_metallicRoughness"},
                {"_diff_", "_arm_"},
                {"_diff", "_arm"},
                {"_diffuse", "_arm"},
                {"_albedo", "_arm"},
                {"_albedo", "_metallicRoughness"}};

            for (const auto& [target, replacement] : patterns)
            {
                std::size_t pos = diff_path.find(target);
                if (pos != std::string::npos)
                {
                    std::string mr_path = diff_path;
                    mr_path.replace(pos, target.length(), replacement);
                    const std::string full_mr_path =
                        resolve_asset_path(mr_path, state.filename);
                    if (std::filesystem::exists(full_mr_path))
                    {
                        metallic_roughness_map_id = load_texture_id(
                            state.s, state.texture_ids, full_mr_path, false);
                        break;
                    }

                    std::size_t ext_pos = mr_path.rfind('.');
                    if (ext_pos != std::string::npos)
                    {
                        std::string mr_path_png = mr_path;
                        mr_path_png.replace(
                            ext_pos, mr_path_png.length() - ext_pos, ".png");
                        const std::string full_mr_path_png =
                            resolve_asset_path(mr_path_png, state.filename);
                        if (std::filesystem::exists(full_mr_path_png))
                        {
                            metallic_roughness_map_id =
                                load_texture_id(state.s, state.texture_ids,
                                                full_mr_path_png, false);
                            break;
                        }
                    }
                }
            }
        }
    }
    state.s.materials.emplace_back(
        to_material(src, texture_id, normal_map_id, metallic_roughness_map_id));
    const std::size_t id = state.s.materials.size() - 1;
    state.ids.emplace(src.name, id);
    const auto& m = state.s.materials.back();
    const std::string tex_label =
        m.texture_id ? std::to_string(*m.texture_id) : "none";
    const std::string norm_label =
        m.normal_map_id ? std::to_string(*m.normal_map_id) : "none";
    const std::string mr_label =
        m.metallic_roughness_map_id
            ? std::to_string(*m.metallic_roughness_map_id)
            : "none";
    std::println(
        "parsed mtl material name={} id={} Ka=({}, {}, {}) "
        "Kd=({}, {}, {}) Ks=({}, {}, {}) Ns={} tex_id={} norm_id={} mr_id={}",
        src.name, id, m.ambient.x, m.ambient.y, m.ambient.z, m.diffuse.x,
        m.diffuse.y, m.diffuse.z, m.specular.x, m.specular.y, m.specular.z,
        m.shininess, tex_label, norm_label, mr_label);
}

void flush_mtl_material(mtl_state& state)
{
    register_mtl_material(state);
}

void handle_newmtl(std::istringstream& ss, mtl_state& state)
{
    flush_mtl_material(state);
    ss >> state.current.name;
    state.current.ambient.reset();
    state.current.diffuse.reset();
    state.current.specular.reset();
    state.current.emissive.reset();
    state.current.shininess.reset();
    state.current.dissolve.reset();
    state.current.illum.reset();
    state.current.diffuse_map.reset();
    state.current.specular_map.reset();
    state.current.normal_map.reset();
    state.current.metalness.reset();
    state.current.roughness.reset();
    state.current.metallic_roughness_map.reset();
}

void handle_ka(std::istringstream& ss, mtl_state& state)
{
    state.current.ambient = parse_vec3_optional(ss);
}

void handle_kd(std::istringstream& ss, mtl_state& state)
{
    state.current.diffuse = parse_vec3_optional(ss);
}

void handle_ks(std::istringstream& ss, mtl_state& state)
{
    state.current.specular = parse_vec3_optional(ss);
}

void handle_ns(std::istringstream& ss, mtl_state& state)
{
    state.current.shininess = parse_float_optional(ss);
}

void handle_ke(std::istringstream& ss, mtl_state& state)
{
    state.current.emissive = parse_vec3_optional(ss);
}

void handle_d(std::istringstream& ss, mtl_state& state)
{
    state.current.dissolve = parse_float_optional(ss);
}

void handle_tr(std::istringstream& ss, mtl_state& state)
{
    const auto tr = parse_float_optional(ss);
    if (tr)
        state.current.dissolve = 1. - *tr;
}

void handle_illum(std::istringstream& ss, mtl_state& state)
{
    int illum_model = 2;
    if (ss >> illum_model)
        state.current.illum = illum_model;
}

void handle_ni(std::istringstream& ss, mtl_state& state)
{
    state.current.ior = parse_float_optional(ss);
}

void handle_map_kd(std::istringstream& ss, mtl_state& state)
{
    std::string map_path;
    ss >> map_path;
    if (!map_path.empty())
        state.current.diffuse_map = map_path;
}

void handle_map_ks(std::istringstream& ss, mtl_state& state)
{
    std::string map_path;
    ss >> map_path;
    if (!map_path.empty())
        state.current.specular_map = map_path;
}

void handle_bump(std::istringstream& ss, mtl_state& state)
{
    std::string map_path;
    ss >> map_path;
    if (map_path == "-bm")
    {
        double multiplier;
        ss >> multiplier;
        ss >> map_path;
    }
    if (!map_path.empty())
        state.current.normal_map = map_path;
}

void handle_pm(std::istringstream& ss, mtl_state& state)
{
    state.current.metalness = parse_float_optional(ss);
}

void handle_pr(std::istringstream& ss, mtl_state& state)
{
    state.current.roughness = parse_float_optional(ss);
}

void handle_map_pm(std::istringstream& ss, mtl_state& state)
{
    std::string map_path;
    ss >> map_path;
    if (!map_path.empty())
        state.current.metallic_roughness_map = map_path;
}

void handle_map_pr(std::istringstream& ss, mtl_state& state)
{
    std::string map_path;
    ss >> map_path;
    if (!map_path.empty())
        state.current.metallic_roughness_map = map_path;
}

static const std::unordered_map<std::string, mtl_handler_t>
    g_mtl_dispatch_table = {
        {"newmtl", handle_newmtl}, {"Ka", handle_ka},
        {"Kd", handle_kd},         {"Ks", handle_ks},
        {"Ns", handle_ns},         {"Ke", handle_ke},
        {"d", handle_d},           {"Tr", handle_tr},
        {"illum", handle_illum},   {"Ni", handle_ni},
        {"Pm", handle_pm},         {"Pr", handle_pr},
        {"map_Kd", handle_map_kd}, {"map_Ks", handle_map_ks},
        {"map_Bump", handle_bump}, {"map_bump", handle_bump},
        {"bump", handle_bump},     {"map_Kn", handle_bump},
        {"norm", handle_bump},     {"map_Pm", handle_map_pm},
        {"map_Pr", handle_map_pr}};

void parse_mtl_lines(std::ifstream& f, mtl_state& state)
{
    std::string line;
    while (std::getline(f, line))
    {
        parse_utils::trim_line(line);
        if (parse_utils::should_skip_line(line))
            continue;

        std::istringstream ss(line);
        std::string type;
        if (!(ss >> type))
            continue;

        const auto it = g_mtl_dispatch_table.find(type);
        if (it != g_mtl_dispatch_table.end())
        {
            it->second(ss, state);
        }
    }
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

    mtl_state state{filename, s, ids, texture_ids, mtl_material{}};
    parse_mtl_lines(f, state);
    flush_mtl_material(state);
}

// ---------------------------------------------------------
// OBJ Helper Functions and Handlers
// ---------------------------------------------------------

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
    const double len_sq = glm::dot(v, v);
    if (len_sq <= kNormalEpsLen2)
        return vec3{0., 0., 0.};
    return v * glm::inversesqrt(len_sq);
}

[[nodiscard]] vec2 parse_vec2(std::istringstream& ss)
{
    vec2 v;
    if (!(ss >> v.x >> v.y))
        return {0., 0.};
    return v;
}

[[nodiscard]] int parse_vertex_index(std::string_view part,
                                     std::size_t vertex_count)
{
    if (part.empty())
        throw std::runtime_error{"obj parser: malformed face vertex index"};
    const int raw_vertex = std::stoi(std::string{part});
    const int vertex_idx = obj_to_zero_based(raw_vertex, vertex_count);
    if (vertex_idx < 0 || vertex_idx >= static_cast<int>(vertex_count))
    {
        throw std::runtime_error{std::format(
            "obj parser: vertex index {} out of range", raw_vertex)};
    }
    return vertex_idx;
}

[[nodiscard]] int parse_uv_index(std::string_view part, std::size_t uv_count)
{
    if (part.empty())
        return kMissingIndex;
    const int raw_uv = std::stoi(std::string{part});
    const int uv_idx = obj_to_zero_based(raw_uv, uv_count);
    if (uv_idx < 0 || uv_idx >= static_cast<int>(uv_count))
    {
        throw std::runtime_error{
            std::format("obj parser: uv index {} out of range", raw_uv)};
    }
    return uv_idx;
}

[[nodiscard]] int parse_normal_index(std::string_view part,
                                     std::size_t normal_count)
{
    if (part.empty())
        return kMissingIndex;
    const int raw_normal = std::stoi(std::string{part});
    const int normal_idx = obj_to_zero_based(raw_normal, normal_count);
    if (normal_idx < 0 || normal_idx >= static_cast<int>(normal_count))
    {
        throw std::runtime_error{std::format(
            "obj parser: normal index {} out of range", raw_normal)};
    }
    return normal_idx;
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
    const int vertex_idx = parse_vertex_index(vertex_part, vertex_count);

    int uv_idx = kMissingIndex;
    if (first_slash != std::string_view::npos)
    {
        const auto uv_part =
            token.substr(first_slash + 1, second_slash - first_slash - 1);
        uv_idx = parse_uv_index(uv_part, uv_count);
    }

    int normal_idx = kMissingIndex;
    if (second_slash != std::string_view::npos &&
        second_slash + 1 < token.size())
    {
        const auto normal_part = token.substr(second_slash + 1);
        normal_idx = parse_normal_index(normal_part, normal_count);
    }

    return face_vertex_ref{
        .vertex_idx = vertex_idx, .uv_idx = uv_idx, .normal_idx = normal_idx};
}

void parse_face(std::istringstream& ss, obj_state& state)
{
    state.face_offsets.push_back(state.face_vertex_indices.size());
    std::string token;
    std::size_t count = 0;
    while (ss >> token)
    {
        const auto ref =
            parse_face_token(token, state.obj_positions.size(),
                             state.obj_uvs.size(), state.obj_normals.size());
        state.face_vertex_indices.push_back(ref.vertex_idx);
        state.face_uv_indices.push_back(ref.uv_idx);
        state.face_normal_indices.push_back(ref.normal_idx);
        count++;
    }
    if (count >= 3)
    {
        state.face_material_ids.push_back(state.current_material_id);
    }
    else
    {
        state.face_offsets.pop_back();
        state.face_vertex_indices.resize(state.face_vertex_indices.size() -
                                         count);
        state.face_uv_indices.resize(state.face_uv_indices.size() - count);
        state.face_normal_indices.resize(state.face_normal_indices.size() -
                                         count);
    }
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

void handle_vertex(std::istringstream& ss, obj_state& state)
{
    state.obj_positions.push_back(parse_vec3(ss) + state.origin);
}

void handle_uv(std::istringstream& ss, obj_state& state)
{
    state.obj_uvs.push_back(parse_vec2(ss));
}

void handle_normal(std::istringstream& ss, obj_state& state)
{
    state.obj_normals.push_back(safe_normalize(parse_vec3(ss)));
}

void handle_mtllib(std::istringstream& ss, obj_state& state)
{
    std::string mtl_token;
    while (ss >> mtl_token)
    {
        parse_mtl_file(resolve_mtl_path(mtl_token, state.filename), state.s,
                       state.mtl_ids, state.texture_ids);
    }
}

void handle_usemtl(std::istringstream& ss, obj_state& state)
{
    std::string material_name;
    ss >> material_name;
    state.current_material_id =
        resolve_material_id(material_name, state.mtl_ids, state.material_id);
}

void handle_face(std::istringstream& ss, obj_state& state)
{
    parse_face(ss, state);
}

static const std::unordered_map<std::string, obj_handler_t>
    g_obj_dispatch_table = {
        {"v", handle_vertex},      {"vt", handle_uv},
        {"vn", handle_normal},     {"f", handle_face},
        {"mtllib", handle_mtllib}, {"usemtl", handle_usemtl}};

void parse_obj_lines(std::ifstream& f, obj_state& state)
{
    std::string line;
    while (std::getline(f, line))
    {
        parse_utils::trim_line(line);
        if (parse_utils::should_skip_line(line))
            continue;

        std::istringstream ss(line);
        std::string type;
        if (!(ss >> type))
            continue;

        const auto it = g_obj_dispatch_table.find(type);
        if (it != g_obj_dispatch_table.end())
        {
            it->second(ss, state);
        }
    }
}

void accumulate_face_normals(const obj_state& state, std::size_t start,
                             std::size_t end,
                             std::vector<vec3>& generated_normals)
{
    const std::size_t num_verts = end - start;
    for (std::size_t i = 1; i + 1 < num_verts; ++i)
    {
        const vec3& a =
            state.obj_positions.at(state.face_vertex_indices[start]);
        const vec3& b =
            state.obj_positions.at(state.face_vertex_indices[start + i]);
        const vec3& c =
            state.obj_positions.at(state.face_vertex_indices[start + i + 1]);
        const vec3 fn = face_normal(a, b, c);
        generated_normals[state.face_vertex_indices[start]] += fn;
        generated_normals[state.face_vertex_indices[start + i]] += fn;
        generated_normals[state.face_vertex_indices[start + i + 1]] += fn;
    }
}

[[nodiscard]] std::vector<vec3> build_generated_normals(const obj_state& state)
{
    std::vector<vec3> generated_normals(state.obj_positions.size(),
                                        vec3{0., 0., 0.});
    const std::size_t num_faces = state.face_offsets.size();
    for (std::size_t j = 0; j < num_faces; ++j)
    {
        const std::size_t start = state.face_offsets[j];
        const std::size_t end = (j + 1 < num_faces)
                                    ? state.face_offsets[j + 1]
                                    : state.face_vertex_indices.size();
        accumulate_face_normals(state, start, end, generated_normals);
    }
    std::transform(generated_normals.begin(), generated_normals.end(),
                   generated_normals.begin(), safe_normalize);
    return generated_normals;
}

[[nodiscard]] vec3 pick_vertex_normal(
    int normal_idx, int vert_idx, const std::vector<vec3>& obj_normals,
    const std::vector<vec3>& generated_normals, const vec3& fallback)
{
    if (normal_idx >= 0)
        return obj_normals.at(normal_idx);
    const vec3 generated = generated_normals.at(vert_idx);
    if (glm::dot(generated, generated) > kNormalEpsLen2)
        return generated;
    if (glm::dot(fallback, fallback) > kNormalEpsLen2)
        return fallback;
    return vec3{0., 1., 0.};
}

[[nodiscard]] vec2 pick_vertex_uv(int uv_idx, const std::vector<vec2>& obj_uvs)
{
    if (uv_idx >= 0)
        return obj_uvs.at(uv_idx);
    return vec2{0., 0.};
}

void push_triangulated_triangle(scene& s, const obj_state& state,
                                const std::vector<vec3>& generated_normals,
                                std::size_t start, std::size_t i,
                                std::size_t mat_id)
{
    const int idx0 = state.face_vertex_indices[start];
    const int idx1 = state.face_vertex_indices[start + i];
    const int idx2 = state.face_vertex_indices[start + i + 1];

    const vec3& p0 = state.obj_positions.at(idx0);
    const vec3& p1 = state.obj_positions.at(idx1);
    const vec3& p2 = state.obj_positions.at(idx2);
    const vec3 fallback = face_normal(p0, p1, p2);

    const auto base_vertex = s.vertices.size();
    s.vertices.push_back(vertex{
        .p = p0,
        .n = pick_vertex_normal(state.face_normal_indices[start], idx0,
                                state.obj_normals, generated_normals, fallback),
        .uv = pick_vertex_uv(state.face_uv_indices[start], state.obj_uvs),
    });
    s.vertices.push_back(vertex{
        .p = p1,
        .n = pick_vertex_normal(state.face_normal_indices[start + i], idx1,
                                state.obj_normals, generated_normals, fallback),
        .uv = pick_vertex_uv(state.face_uv_indices[start + i], state.obj_uvs),
    });
    s.vertices.push_back(vertex{
        .p = p2,
        .n = pick_vertex_normal(state.face_normal_indices[start + i + 1], idx2,
                                state.obj_normals, generated_normals, fallback),
        .uv =
            pick_vertex_uv(state.face_uv_indices[start + i + 1], state.obj_uvs),
    });

    s.mesh_triangles.push_back(triangle{
        .vertex_start = base_vertex,
        .material_id = mat_id,
    });
}

void triangulate_and_push_vertices(scene& s, const obj_state& state,
                                   const std::vector<vec3>& generated_normals)
{
    const std::size_t num_faces = state.face_offsets.size();
    for (std::size_t j = 0; j < num_faces; ++j)
    {
        const std::size_t start = state.face_offsets[j];
        const std::size_t end = (j + 1 < num_faces)
                                    ? state.face_offsets[j + 1]
                                    : state.face_vertex_indices.size();
        const std::size_t num_verts = end - start;
        const std::size_t mat_id = state.face_material_ids[j];

        for (std::size_t i = 1; i + 1 < num_verts; ++i)
        {
            push_triangulated_triangle(s, state, generated_normals, start, i,
                                       mat_id);
        }
    }
}

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
        throw std::runtime_error{std::format(
            "obj parser: material out of bounds: size is {} and id is {}",
            material_id, s.materials.size())};
    }

    obj_state state{filename, s, origin, material_id};
    state.current_material_id = material_id;

    parse_obj_lines(f, state);

    const auto generated_normals = build_generated_normals(state);
    const auto first_tri = s.mesh_triangles.size();

    triangulate_and_push_vertices(s, state, generated_normals);

    const auto tri_count = s.mesh_triangles.size() - first_tri;
    std::println("done loading model (vertex count: {}, face count: {})",
                 state.obj_positions.size(), tri_count);

    mesh3d& new_mesh = std::get<mesh3d>(
        s.objects.emplace_back(mesh3d{first_tri, tri_count, material_id}));
    build_bvh(new_mesh.blas,
              std::span{s.mesh_triangles}.subspan(new_mesh.triangle_start,
                                                  new_mesh.triangle_count),
              s.vertices);
}

} // namespace

void parse_obj_file(std::istringstream& ss, scene& s)
{
    std::string filename_token;
    ss >> filename_token;
    const std::string filename = resolve_obj_path(filename_token, s.source_dir);

    vec3 origin{0, 0, 0};
    double ox{}, oy{}, oz{};
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
