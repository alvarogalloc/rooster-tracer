export module material;
import std;
import glm;
import color_rgb;
import hitevent;

export namespace cg
{
struct material
{
    color_rgb ambient;
    color_rgb specular;
    color_rgb diffuse;
    double shininess;
    double reflectivity{0.0};
    double transparency{0.0};
    double ior{1.0};
    std::optional<std::size_t> texture_id{};
    std::optional<std::size_t> normal_map_id{};
    double metalness{0.0};
    double roughness{0.5};
    std::optional<std::size_t> metallic_roughness_map_id{};
};
constexpr double kDefaultShininess = 10.0;
constexpr double kDefaultAmbientFactor = 0.1;

[[nodiscard]] material make_phong_material(
    color_rgb albedo, double shininess = kDefaultShininess);
[[nodiscard]] material make_textured_material(
    std::size_t texture_id, color_rgb specular,
    double shininess = kDefaultShininess);

[[nodiscard]] color_rgb phong_brdf(const material& m, const hitevent& he,
                                   vec3 light_dir, color_rgb radiance,
                                   vec3 view_dir);

using material_collection = std::vector<material>;
} // namespace cg
