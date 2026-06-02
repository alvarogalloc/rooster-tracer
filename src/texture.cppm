export module texture;
import std;
import glm;
import color_rgb;

export namespace cg
{
struct texture
{
    int width{};
    int height{};
    std::vector<color_rgb> pixels{};
};

[[nodiscard]] texture load_texture(const std::string& path, bool is_srgb = true);
[[nodiscard]] texture load_hdr(const std::string& path);
[[nodiscard]] color_rgb sample_texture(const texture& tex, vec2 uv);
} // namespace cg
