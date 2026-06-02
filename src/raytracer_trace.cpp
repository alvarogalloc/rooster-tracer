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

// implement hdri support, with fallback to trad bg
[[nodiscard]] color_rgb sample_environment(const scene& scene_data, vec3 dir)
{
    if (!scene_data.environment)
        return scene_data.background_color;

    const texture& env = *scene_data.environment;
    const double theta = std::acos(std::clamp(dir.y, -1.0, 1.0));
    const double phi = std::atan2(dir.z, dir.x);
    const double u = phi * (0.5 / std::numbers::pi) + 0.5;
    const double v = 1.0 - theta * (1.0 / std::numbers::pi);
    return sample_texture(env, vec2{u, v});
}

// check here https://en.wikipedia.org/wiki/Schlick%27s_approximation
[[nodiscard]] double schlick_reflectance(double cos_theta, double ior)
{
    double r0 = (1.0 - ior) / (1.0 + ior);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * std::pow(1.0 - cos_theta, 5);
}

cg::material apply_texture_and_normals(const cg::scene& scene_data,
                                       const cg::material& base,
                                       cg::hitevent& hit)
{
    cg::material shaded = base;
    if (!base.texture_id && !base.normal_map_id &&
        !base.metallic_roughness_map_id)
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
    if (base.metallic_roughness_map_id)
    {
        const std::size_t mrid = *base.metallic_roughness_map_id;
        if (mrid < scene_data.textures.size())
        {
            const cg::color_rgb mr_color =
                cg::sample_texture(scene_data.textures.at(mrid), hit.uv);
            // Green channel contains roughness, blue channel contains
            // metalness.
            shaded.roughness = base.roughness * mr_color.y;
            shaded.metalness = base.metalness * mr_color.z;
        }
    }
    return shaded;
}

std::optional<hitevent> find_closest_hit(const scene& scene_data, ray ray_data);

bool is_occluded(const cg::scene& scene_data, cg::ray ray_data,
                 cg::interval hit_range)
{
    std::optional<cg::hitevent> shadow_hit =
        find_closest_hit(scene_data, ray_data);
    if (shadow_hit)
    {
        const cg::material& mat = scene_data.materials.at(shadow_hit->m_id);
        if (mat.transparency < 0.5)
        {
            // If it's mostly solid, it casts a shadow
            return true;
        }
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

color_rgb shade_hit(const scene& scene_data, const hitevent& hit,
                    const material& material_shaded, vec3 view_dir)
{
    color_rgb result =
        material_shaded.ambient * (1.0 - material_shaded.metalness);

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
        return color_rgb{0.01, 0.01,
                         0.01}; // super dark grey for trapped shadows

    const auto closest_hit = find_closest_hit(scene_data, ray_data);
    if (!closest_hit)
        return sample_environment(scene_data, ray_data.dir);

    hitevent hit = *closest_hit;
    const material& base_mat = scene_data.materials.at(hit.m_id);
    const material mat = apply_texture_and_normals(scene_data, base_mat, hit);

    const color_rgb direct_color =
        shade_hit(scene_data, hit, mat, -ray_data.dir);

    const double metalness = mat.metalness;
    const double active_reflectivity = std::max(mat.reflectivity, metalness);
    const color_rgb active_specular =
        color_rgb{glm::mix(static_cast<vec3>(mat.specular),
                           static_cast<vec3>(mat.diffuse), metalness)};

    color_rgb indirect_color{0.0, 0.0, 0.0};
    double indirect_weight = 0.0;

    constexpr double kRayBias = 1e-4;

    // Specular Reflection (Mirror)
    if (active_reflectivity > 0.0 && mat.transparency == 0.0)
    {
        const vec3 reflected_dir =
            glm::reflect(ray_data.dir, closest_hit->normal);
        const ray reflected_ray{closest_hit->p + kRayBias * closest_hit->normal,
                                reflected_dir};
        const color_rgb reflected_color =
            trace_ray(scene_data, reflected_ray, depth - 1);

        indirect_color += reflected_color * active_specular;
        indirect_weight += active_reflectivity;
    }

    // Glass (Specular Reflection + Refraction)
    if (mat.transparency > 0.0)
    {
        const double cos_i = glm::dot(ray_data.dir, closest_hit->normal);
        const bool entering = cos_i < 0.0;
        const vec3 outward_normal =
            entering ? closest_hit->normal : -closest_hit->normal;
        const double eta = entering ? (1.0 / mat.ior) : (mat.ior / 1.0);
        const double cos_theta =
            std::min(-glm::dot(ray_data.dir, outward_normal), 1.0);

        const vec3 refracted_dir =
            glm::refract(ray_data.dir, outward_normal, eta);

        if (glm::length2(refracted_dir) <
            0.1) // Total Internal Reflection (TIR)
        {
            const vec3 reflected_dir =
                glm::reflect(ray_data.dir, closest_hit->normal);
            const ray reflected_ray{closest_hit->p + kRayBias * outward_normal,
                                    reflected_dir};
            const color_rgb reflected_color =
                trace_ray(scene_data, reflected_ray, depth - 1);

            indirect_color += mat.transparency * reflected_color * mat.specular;
        }
        else
        {
            // Fresnel / Schlick's weight distribution
            const double R = schlick_reflectance(cos_theta, mat.ior);

            const vec3 reflected_dir =
                glm::reflect(ray_data.dir, closest_hit->normal);
            const ray reflected_ray{closest_hit->p + kRayBias * outward_normal,
                                    reflected_dir};
            // go down in the call path until depth=0 for reflection
            const color_rgb ref_color =
                trace_ray(scene_data, reflected_ray, depth - 1);

            const ray refract_ray{closest_hit->p - kRayBias * outward_normal,
                                  refracted_dir};

            // same for refraction
            const color_rgb trans_color =
                trace_ray(scene_data, refract_ray, depth - 1);

            // blend both colors with the distribution of the
            // schlick_reflectance
            const color_rgb glass_color = color_rgb{
                glm::mix(static_cast<vec3>(trans_color),
                         static_cast<vec3>(ref_color * mat.specular), R)};

            indirect_color += glass_color;
        }
        indirect_weight += mat.transparency;
    }

    if (indirect_weight > 0.0)
    {
        const double w = glm::clamp(indirect_weight, 0.0, 1.0);
        return color_rgb{glm::mix(static_cast<vec3>(direct_color),
                                  static_cast<vec3>(indirect_color), w)};
    }

    return direct_color;
}
} // namespace cg
