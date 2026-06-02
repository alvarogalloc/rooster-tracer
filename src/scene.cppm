export module scene;
import std;

import light;
import material;
import variant_overload;
import triangle;
import ray;
import interval;
import sphere;
import mesh3d;
import glm;
import color_rgb;
import plane;
import camera;
import texture;
export namespace cg
{
struct scene
{
    using primitive_t = std::variant<sphere, triangle, plane, mesh3d>;
    std::vector<primitive_t> objects{};
    light_collection lights{};
    material_collection materials{};
    std::vector<texture> textures{};
    std::vector<vertex> vertices{};
    std::vector<triangle> mesh_triangles{};
    std::string source_dir{};
    std::optional<texture> environment{};
    camera camera_data{};
    color_rgb background_color{color_rgb::from_rgb_256(10, 32, 90)};
    u32 max_depth{5};
};

constexpr auto primitive_visitor(const scene& scene_data, const ray& ray_data,
                                 const interval& hit_range)
{
    return overload{
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
    };
}
} // namespace cg
