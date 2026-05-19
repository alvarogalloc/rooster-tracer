module raytracer;
import triangle;
import mesh3d;
import sphere;
import light;
import directional_light;
import point_light;
import material;
import plane;
import interval;
import glm;
import std;

namespace
{
template <class... Ts> struct overload : Ts...
{
  using Ts::operator()...;
};

constexpr float kShadowBias = 1e-3f;

bool is_occluded(const cg::scene& scene_data, cg::ray ray_data,
                 cg::interval hit_range)
{
  for (const auto& object : scene_data.objects)
  {
    const bool hit =
        object
            .visit(overload{
                [&](const cg::triangle& tri) {
                  return get_ray_triangle_hit(tri, scene_data.vertices, ray_data,
                                              hit_range);
                },
                [&](const cg::sphere& sph) {
                  return get_ray_sphere_hit(sph, ray_data, hit_range);
                },
                [&](const cg::mesh3d& mesh) {
                  return get_ray_mesh_hit(mesh, scene_data.mesh_triangles,
                                          scene_data.vertices, ray_data,
                                          hit_range);
                },
                [&](const cg::plane& p) {
                  return get_ray_plane_hit(p, ray_data, hit_range);
                },
            })
            .has_value();
    if (hit)
      return true;
  }
  return false;
}
} // namespace

namespace cg
{
std::optional<hitevent> find_closest_hit(const scene& scene_data, ray ray_data)
{
  std::optional<hitevent> closest_hit;
  const interval hit_range{scene_data.camera_data.near,
                           scene_data.camera_data.far};
  for (const auto& object : scene_data.objects)
  {
    std::optional<hitevent> hit = object.visit(overload{
        [&](const triangle& tri) {
          return get_ray_triangle_hit(tri, scene_data.vertices, ray_data,
                                      hit_range);
        },
        [&](const sphere& sph) {
          return get_ray_sphere_hit(sph, ray_data, hit_range);
        },
        [&](const mesh3d& mesh) {
          return get_ray_mesh_hit(mesh, scene_data.mesh_triangles,
                                  scene_data.vertices, ray_data, hit_range);
        },
        [&](const plane& p) {
          return get_ray_plane_hit(p, ray_data, hit_range);
        },
    });

    if (!hit || (closest_hit && hit->t >= closest_hit->t))
      continue;
    closest_hit = hit;
  }
  return closest_hit;
}

color_rgb shade_hit(const scene& scene_data, const hitevent& hit, vec3 view_dir)
{
  if (hit.m_id >= scene_data.materials.size())
  {
    throw std::runtime_error{
        std::format("invalid material id {} for hit event", hit.m_id)};
  }

  const material& material_data = scene_data.materials.at(hit.m_id);
  color_rgb result = material_data.ambient;
  for (const auto& light_data : scene_data.lights)
  {
    const bool blocked = std::visit(
        overload{
            [&](const directional_light& l) {
              const vec3 shadow_origin = hit.p + hit.normal * kShadowBias;
              const vec3 light_dir = -l.dir;
              const ray shadow_ray{shadow_origin, light_dir};
              const interval shadow_range{
                  kShadowBias, std::numeric_limits<float>::infinity()};
              return is_occluded(scene_data, shadow_ray, shadow_range);
            },
            [&](const point_light& l) {
              const vec3 to_light = l.pos - hit.p;
              const float light_distance = glm::length(to_light);
              if (light_distance <= kShadowBias)
                return false;
              const vec3 shadow_origin = hit.p + hit.normal * kShadowBias;
              const ray shadow_ray{shadow_origin, to_light};
              const interval shadow_range{kShadowBias,
                                          light_distance - kShadowBias};
              return is_occluded(scene_data, shadow_ray, shadow_range);
            },
        },
        light_data);
    if (blocked)
      continue;

    result +=
        std::visit(overload{
                       [&](const directional_light& l) {
                         return shade_phong(material_data, hit, l, view_dir);
                       },
                       [&](const point_light& l) {
                         return shade_phong(material_data, hit, l, view_dir);
                       },
                   },
                   light_data);
  }
  return result;
}

color_rgb trace_ray(const scene& scene_data, ray ray_data, int depth)
{
  if (depth <= 0)
    return scene_data.background_color;

  const auto closest_hit = find_closest_hit(scene_data, ray_data);
  if (!closest_hit)
    return scene_data.background_color;

  return shade_hit(scene_data, *closest_hit, -ray_data.dir);
}
} // namespace cg
