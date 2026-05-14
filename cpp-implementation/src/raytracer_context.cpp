module raytracer;
import std;
import glm;

namespace cg
{
std::optional<std::string> validate_scene_for_render(const scene& scene_data)
{
  const auto& camera_data = scene_data.camera_data;
  if (camera_data.width <= 0 || camera_data.height <= 0)
    return "viewport width and height must be > 0";
  if (camera_data.fov <= 0.f || camera_data.fov >= glm::pi<float>())
    return "fov must be in (0, PI) radians";
  if (camera_data.near <= 0.f)
    return "near plane must be > 0";
  if (camera_data.far <= camera_data.near)
    return "far plane must be greater than near plane";
  if (scene_data.max_depth == 0)
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
