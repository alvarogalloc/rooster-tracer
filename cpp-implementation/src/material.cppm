export module material;
import std;
import glm;
import color_rgb;
import directional_light;
import point_light;
import hitevent;

export namespace cg
{
struct material
{
    color_rgb ambient;
    color_rgb specular;
    color_rgb diffuse;
    float shininess;
    std::optional<std::size_t> texture_id{};
    std::optional<std::size_t> normal_map_id{};
};
constexpr float kDefaultShininess = 32.f;
constexpr float kDefaultAmbientFactor = 0.1f;
constexpr float kMinDistanceSq = 1e-4f;
constexpr float kMinNormSq = 1e-8f;
[[nodiscard]] material make_phong_material(color_rgb albedo,
                                           float shininess = kDefaultShininess);
[[nodiscard]] material make_textured_material(std::size_t texture_id,
                                              color_rgb specular,
                                              float shininess = kDefaultShininess);

[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const directional_light& l, vec3 view_dir);
[[nodiscard]] color_rgb shade_phong(const material& m, const hitevent& he,
                                    const point_light& l, vec3 view_dir);

using material_collection = std::vector<material>;
} // namespace cg
