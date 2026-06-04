module raytracer;
import triangle;
import mesh3d;
import sphere;
import light;
import directional_light;
import variant_overload;
import point_light;
import spot_light;
import rect_light;
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

    for (const auto& light_data : scene_data.lights)
    {
        result += std::visit(
            overload{
                [&](const rect_light& l) {
                    color_rgb accumulated_contribution{0.0, 0.0, 0.0};
                    std::uint32_t rng_state = static_cast<std::uint32_t>(
                        std::abs(hit.p.x) * 1000.0 +
                        std::abs(hit.p.y) * 100000.0 +
                        std::abs(hit.p.z) * 10000000.0);
                    if (rng_state == 0)
                        rng_state = 1337;
                    auto next_float = [&]() {
                        rng_state = rng_state * 1664525u + 1013904223u;
                        return (rng_state & 0xFFFFFFu) / 16777216.0f;
                    };

                    for (int s = 0; s < l.samples; ++s)
                    {
                        double rand_u = next_float();
                        double rand_v = next_float();
                        vec3 sample_pos = l.pos + rand_u * l.u + rand_v * l.v;

                        vec3 to_light = sample_pos - hit.p;
                        double dist_sq = glm::dot(to_light, to_light);
                        if (dist_sq < 1e-7)
                            continue;
                        double dist = glm::sqrt(dist_sq);
                        vec3 light_dir = to_light / dist;

                        // 1. Check occlusion
                        const vec3 shadow_origin =
                            hit.p + hit.normal * kShadowBias;
                        const ray shadow_ray{shadow_origin, to_light};
                        const interval shadow_range{kShadowBias,
                                                    dist - kShadowBias};
                        if (is_occluded(scene_data, shadow_ray, shadow_range))
                            continue;

                        // 2. Cosine at the light source
                        double cos_light = glm::dot(-light_dir, l.normal);
                        if (cos_light <= 0.0)
                            continue;

                        color_rgb radiance_sample = color_rgb{
                            l.color * (l.intensity / dist_sq) * cos_light /
                            static_cast<double>(l.samples)};
                        accumulated_contribution +=
                            phong_brdf(material_shaded, hit, to_light,
                                       radiance_sample, view_dir);
                    }
                    return accumulated_contribution;
                },
                [&](const point_light& l) {
                    if (l.radius <= 0.0 || l.samples <= 1)
                    {
                        const auto [shadow_ray, shadow_range] = get_shadow_info(l, hit, kShadowBias);
                        if (shadow_range.size() > 0.0 && is_occluded(scene_data, shadow_ray, shadow_range))
                        {
                            return color_rgb{0.0, 0.0, 0.0};
                        }
                        return shade_phong(material_shaded, hit, l, view_dir);
                    }

                    color_rgb accumulated_contribution{0.0, 0.0, 0.0};
                    std::uint32_t rng_state = static_cast<std::uint32_t>(std::abs(hit.p.x) * 1000.0 + std::abs(hit.p.y) * 100000.0 + std::abs(hit.p.z) * 10000000.0);
                    if (rng_state == 0) rng_state = 1337;
                    auto next_float = [&]() {
                        rng_state = rng_state * 1664525u + 1013904223u;
                        return (rng_state & 0xFFFFFFu) / 16777216.0f;
                    };

                    for (int s = 0; s < l.samples; ++s)
                    {
                        double u = next_float();
                        double v = next_float();
                        double theta = u * 2.0 * std::numbers::pi;
                        double phi = std::acos(2.0 * v - 1.0);
                        vec3 offset = vec3(std::sin(phi) * std::cos(theta), std::sin(phi) * std::sin(theta), std::cos(phi)) * l.radius;
                        vec3 sample_pos = l.pos + offset;

                        vec3 to_light = sample_pos - hit.p;
                        double dist_sq = glm::dot(to_light, to_light);
                        if (dist_sq < 1e-7) continue;
                        double dist = glm::sqrt(dist_sq);

                        // Check occlusion
                        const vec3 shadow_origin = hit.p + hit.normal * kShadowBias;
                        const ray shadow_ray{shadow_origin, to_light};
                        const interval shadow_range{kShadowBias, dist - kShadowBias};
                        if (is_occluded(scene_data, shadow_ray, shadow_range))
                            continue;

                        const color_rgb radiance = color_rgb{l.color * (l.intensity / dist_sq) / static_cast<double>(l.samples)};
                        accumulated_contribution += phong_brdf(material_shaded, hit, to_light, radiance, view_dir);
                    }
                    return accumulated_contribution;
                },
                [&](const spot_light& l) {
                    if (l.radius <= 0.0 || l.samples <= 1)
                    {
                        const auto [shadow_ray, shadow_range] = get_shadow_info(l, hit, kShadowBias);
                        if (shadow_range.size() > 0.0 && is_occluded(scene_data, shadow_ray, shadow_range))
                        {
                            return color_rgb{0.0, 0.0, 0.0};
                        }
                        return shade_phong(material_shaded, hit, l, view_dir);
                    }

                    color_rgb accumulated_contribution{0.0, 0.0, 0.0};
                    std::uint32_t rng_state = static_cast<std::uint32_t>(std::abs(hit.p.x) * 1000.0 + std::abs(hit.p.y) * 100000.0 + std::abs(hit.p.z) * 10000000.0);
                    if (rng_state == 0) rng_state = 1337;
                    auto next_float = [&]() {
                        rng_state = rng_state * 1664525u + 1013904223u;
                        return (rng_state & 0xFFFFFFu) / 16777216.0f;
                    };

                    for (int s = 0; s < l.samples; ++s)
                    {
                        double u = next_float();
                        double v = next_float();
                        double theta = u * 2.0 * std::numbers::pi;
                        double phi = std::acos(2.0 * v - 1.0);
                        vec3 offset = vec3(std::sin(phi) * std::cos(theta), std::sin(phi) * std::sin(theta), std::cos(phi)) * l.radius;
                        vec3 sample_pos = l.pos + offset;

                        vec3 to_light = sample_pos - hit.p;
                        double dist_sq = glm::dot(to_light, to_light);
                        if (dist_sq < 1e-7) continue;
                        double dist = glm::sqrt(dist_sq);
                        vec3 light_dir = to_light / dist;

                        // Spotlight direction attenuation
                        const double cos_theta = glm::dot(-light_dir, l.dir);
                        double spot_attenuation = 0.0;
                        if (cos_theta >= l.cos_angle)
                        {
                            spot_attenuation = 1.0;
                        }
                        if (spot_attenuation <= 0.0)
                            continue;

                        // Check occlusion
                        const vec3 shadow_origin = hit.p + hit.normal * kShadowBias;
                        const ray shadow_ray{shadow_origin, to_light};
                        const interval shadow_range{kShadowBias, dist - kShadowBias};
                        if (is_occluded(scene_data, shadow_ray, shadow_range))
                            continue;

                        const color_rgb radiance = color_rgb{l.color * (l.intensity / dist_sq) * spot_attenuation / static_cast<double>(l.samples)};
                        accumulated_contribution += phong_brdf(material_shaded, hit, to_light, radiance, view_dir);
                    }
                    return accumulated_contribution;
                },
                [&](const auto& l) {
                    const auto [shadow_ray, shadow_range] =
                        get_shadow_info(l, hit, kShadowBias);
                    if (shadow_range.size() > 0.0 &&
                        is_occluded(scene_data, shadow_ray, shadow_range))
                    {
                        return color_rgb{0.0, 0.0, 0.0};
                    }
                    return shade_phong(material_shaded, hit, l, view_dir);
                }},
            light_data);
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
