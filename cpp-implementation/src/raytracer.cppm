export module raytracer;
import std;
import scene;
import camera;
import color_rgb;
import ray;
import hitevent;
import glm;

export namespace cg
{
struct render_context
{
  const scene& scene_data;
  camera camera_data;
  color_rgb background_color;
  u32 max_depth;
};

[[nodiscard]] render_context make_render_context(const scene& scene_data);

[[nodiscard]] std::optional<std::string> validate_scene_for_render(
    const scene& scene_data);

[[nodiscard]] std::optional<hitevent> find_closest_hit(
    const scene& scene_data, const camera& camera_data, ray ray_data);

[[nodiscard]] color_rgb shade_hit(const scene& scene_data, const hitevent& hit,
                                  vec3 view_dir);

[[nodiscard]] color_rgb trace_ray(const scene& scene_data,
                                  const camera& camera_data,
                                  color_rgb background_color, ray ray_data,
                                  int depth);

void render_scene(const render_context& context_data,
                  std::span<color_rgb> image);

void save_png(std::string_view path, int width, int height,
              std::span<const color_rgb> image);

void render_to_png(const render_context& context_data, std::string_view path);
} // namespace cg
