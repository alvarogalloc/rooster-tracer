module raytracer;
import triangle;
import mesh3d;
import sphere;
import light;
import directional_light;
import variant_overload;
import point_light;
import material;
import plane;
import texture;
import interval;
import glm;
import std;

namespace
{

using namespace cg;
cg::material apply_texture_and_normals(const cg::scene& scene_data,
                                       const cg::material& base,
                                       cg::hitevent& hit)
{
    cg::material shaded = base;
    if (!base.texture_id && !base.normal_map_id)
        return base;

    if (base.texture_id)
    {

        const std::size_t texid = *base.texture_id;
        // texture map
        if (texid >= scene_data.textures.size())
            return base;
        const cg::color_rgb tex_color =
            cg::sample_texture(scene_data.textures.at(texid), hit.uv);
        shaded.diffuse = cg::color_rgb{base.diffuse * tex_color};
        shaded.ambient = cg::color_rgb{base.ambient * tex_color};
    }
    if (base.normal_map_id)
    {

        const std::size_t mapid = *base.normal_map_id;
        // normal map
        if (mapid >= scene_data.textures.size())
            return base;

        const cg::color_rgb norm_color =
            cg::sample_texture(scene_data.textures.at(mapid), hit.uv);

        // Map [0, 1] to [-1, 1]
        vec3 map_n =
            vec3(norm_color.x, norm_color.y, norm_color.z) * 2.0 - vec3(1.0);

        // Gram-Schmmapidt orthogonalization for TBN
        vec3 t = glm::normalize(hit.dpdu -
                                hit.normal * glm::dot(hit.dpdu, hit.normal));
        vec3 b = glm::cross(hit.normal, t);

        // Check handedness
        if (glm::dot(b, hit.dpdv) < 0.0)
        {
            b = -b;
        }

        glm::mat3 tbn{t, b, hit.normal};
        hit.normal = glm::normalize(tbn * map_n);
    }
    return shaded;
}

bool is_occluded(const cg::scene& scene_data, cg::ray ray_data,
                 cg::interval hit_range)
{
    const auto visitor = cg::primitive_visitor(scene_data, ray_data, hit_range);
    for (const auto& object : scene_data.objects)
    {
        const bool hit = std::visit(visitor, object).has_value();
        if (hit)
            return true;
    }
    return false;
}
std::optional<hitevent> find_closest_hit(const scene& scene_data, ray ray_data)
{
    std::optional<hitevent> closest_hit;
    const interval hit_range{scene_data.camera_data.near,
                             scene_data.camera_data.far};
    const auto visitor = primitive_visitor(scene_data, ray_data, hit_range);
    for (const auto& object : scene_data.objects)
    {
        std::optional<hitevent> hit = std::visit(visitor, object);

        if (!hit || (closest_hit && hit->t >= closest_hit->t))
            continue;
        closest_hit = hit;
    }
    return closest_hit;
}

color_rgb shade_hit(const scene& scene_data, const hitevent& hit_in,
                    vec3 view_dir)
{
    if (hit_in.m_id >= scene_data.materials.size())
    {
        throw std::runtime_error{
            std::format("invalid material id {} for hit event", hit_in.m_id)};
    }

    hitevent hit = hit_in;
    const material& material_data = scene_data.materials.at(hit.m_id);
    const material material_shaded =
        apply_texture_and_normals(scene_data, material_data, hit);
    color_rgb result = material_shaded.ambient;

    constexpr static double kShadowBias = 1e-5;
    const auto occlusion_visitor = overload{
        [&](const auto& l) {
            const auto [shadow_ray, shadow_range] =
                get_shadow_info(l, hit, kShadowBias);
            if (shadow_range.size() == 0.)
                return false;
            return is_occluded(scene_data, shadow_ray, shadow_range);
        },
    };

    auto shade_fn = shade_visitor(hit, material_shaded, view_dir);
    for (const auto& light_data : scene_data.lights)
    {
        const bool is_blocked = std::visit(occlusion_visitor, light_data);

        if (is_blocked)
            continue;

        result += std::visit(shade_fn, light_data);
    }
    return result;
}

} // namespace

namespace cg
{

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
