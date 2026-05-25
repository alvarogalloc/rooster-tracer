module texture;
import std;
import stb;

namespace
{
using namespace cg;

[[nodiscard]] double wrap_uv(double v)
{
    const double wrapped = v - std::floor(v);
    return wrapped < 0. ? wrapped + 1. : wrapped;
}

[[nodiscard]] int texel_index(int x, int y, int width)
{
    return y * width + x;
}
} // namespace

namespace cg
{
[[nodiscard]] texture load_texture(const std::string& path)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data =
        stbi_load(path.c_str(), &width, &height, &channels, 3);
    if (!data || width <= 0 || height <= 0)
    {
        throw std::runtime_error{
            std::format("texture load failed: {}", path)};
    }

    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<color_rgb> pixels(pixel_count);
    const double inv = 1. / 255.99;
    for (std::size_t i = 0; i < pixel_count; ++i)
    {
        const std::size_t base = i * 3;
        pixels[i] = color_rgb{inv * vec3{data[base], data[base + 1],
                                         data[base + 2]}};
    }
    stbi_image_free(data);
    return texture{.width = width, .height = height,
                   .pixels = std::move(pixels)};
}

[[nodiscard]] color_rgb sample_texture(const texture& tex, vec2 uv)
{
    if (tex.pixels.empty() || tex.width <= 0 || tex.height <= 0)
        return color_rgb{1., 1., 1.};

    const double u = wrap_uv(uv.x);
    const double v = wrap_uv(uv.y);
    const int x = std::clamp(
        static_cast<int>(u * static_cast<double>(tex.width)), 0,
        tex.width - 1);
    const int y = std::clamp(
        static_cast<int>(v * static_cast<double>(tex.height)), 0,
        tex.height - 1);
    return tex.pixels.at(
        static_cast<std::size_t>(texel_index(x, y, tex.width)));
}
} // namespace cg
