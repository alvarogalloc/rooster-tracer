export module raytracer;
import std;
import scene;
import color_rgb;
import ray;
import hitevent;
import glm;

export namespace cg
{

[[nodiscard]] std::optional<std::string> validate_scene_for_render(
    const scene& scene_data);
[[nodiscard]] color_rgb trace_ray(const scene& scene_data, ray ray_data,
                                  int depth);

void render_scene(const scene& scene_data, std::span<color_rgb> image);

void render_to_png(const scene& scene_data, std::string_view path);
} // namespace cg
