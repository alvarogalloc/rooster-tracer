module raytracer;
import std;
import glm;

namespace cg
{
render_context make_render_context(const scene& scene_data)
{
  const auto& settings = scene_data.settings;
  return render_context{
      .scene_data = scene_data,
      .camera_data =
          camera{
              .width = settings.image_width,
              .height = settings.image_height,
              .fov = settings.fov_radians,
              .pos = settings.camera_pos,
              .up = settings.camera_up,
              .lookAt = settings.camera_look_at,
              .far = settings.far_plane,
              .near = settings.near_plane,
          },
      .background_color = settings.background,
      .max_depth = settings.max_depth,
  };
}

std::optional<std::string> validate_scene_for_render(const scene& scene_data)
{
  const auto& settings = scene_data.settings;
  if (settings.image_width <= 0 || settings.image_height <= 0)
    return "viewport width and height must be > 0";
  if (settings.fov_radians <= 0.f || settings.fov_radians >= glm::pi<float>())
    return "fov must be in (0, PI) radians";
  if (settings.near_plane <= 0.f)
    return "near plane must be > 0";
  if (settings.far_plane <= settings.near_plane)
    return "far plane must be greater than near plane";
  if (settings.max_depth == 0)
    return "max_depth must be >= 1";
  if (scene_data.objects.empty())
    return "scene must contain at least one object";
  if (scene_data.materials.empty())
    return "scene must contain at least one material";
  if (scene_data.lights.empty())
    return "scene must contain at least one light";
  return std::nullopt;
}
} // namespace cg
