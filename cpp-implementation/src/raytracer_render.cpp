module raytracer;
import std;

namespace cg
{
void render_scene(const render_context& context_data, std::span<color_rgb> image)
{
  const auto expected_pixel_count = static_cast<std::size_t>(
      context_data.camera_data.width * context_data.camera_data.height);
  if (image.size() != expected_pixel_count)
  {
    throw std::runtime_error{std::format(
        "image buffer has {} pixels but expected {}", image.size(),
        expected_pixel_count)};
  }

  context_data.camera_data.cast_all_rays([&](ray ray_data, int x, int y) {
    const auto index =
        static_cast<std::size_t>(context_data.camera_data.width * y + x);
    image.at(index) = trace_ray(context_data.scene_data, context_data.camera_data,
                                context_data.background_color, ray_data,
                                static_cast<int>(context_data.max_depth));
  });
}
} // namespace cg
