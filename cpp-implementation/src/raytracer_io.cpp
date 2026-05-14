module raytracer;
import stb;
import std;

namespace cg
{
void save_png(std::string_view path, int width, int height,
              std::span<const color_rgb> image)
{
  constexpr int channels = 3;
  std::vector<u8> pixels;
  pixels.reserve(static_cast<std::size_t>(width) * height * channels);

  for (const color_rgb& c : image)
  {
    auto [r, g, b] = c.to_rgb_255();
    pixels.push_back(r);
    pixels.push_back(g);
    pixels.push_back(b);
  }

  const int stride = width * channels;
  if (!stbi_write_png(path.data(), width, height, channels, pixels.data(), stride))
  {
    throw std::runtime_error{
        std::format("save_png: failed to write '{}'", path)};
  }
}

void render_to_png(const render_context& context_data, std::string_view path)
{
  std::vector<color_rgb> image(static_cast<std::size_t>(context_data.camera_data.width) *
                               context_data.camera_data.height);
  render_scene(context_data, image);
  save_png(path, context_data.camera_data.width, context_data.camera_data.height,
           image);
}
} // namespace cg
