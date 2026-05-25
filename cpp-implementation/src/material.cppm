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
    std::optional<std::size_t> texture_id{};
    std::optional<std::size_t> normal_map_id{};
};
constexpr double kDefaultShininess = 32.0;
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
