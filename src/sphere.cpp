module sphere;
import std;
namespace cg
{

std::optional<hitevent> get_ray_sphere_hit(const sphere& s, ray r, interval i)
{
    const auto dir_to_sphere = s.pos - r.pos;
    auto a = glm::length2(r.dir);
    auto h = glm::dot(r.dir, dir_to_sphere);
    auto c = glm::length2(dir_to_sphere) - s.radius * s.radius;

    auto discriminant = h * h - a * c;

    if (discriminant < 0.)
    {
        return std::nullopt;
    }

    const double sqrt_discriminant = std::sqrt(discriminant);
    double t = (h - sqrt_discriminant) / a;
    if (t < 0. || !i.contains(t))
    {
        t = (h + sqrt_discriminant) / a;
        if (t < 0. || !i.contains(t))
            return std::nullopt;
    }
    auto normal = glm::normalize((r.at(t) - s.pos) / s.radius);
    if (glm::dot(normal, r.dir) > 0.)
    {
        normal = -normal;
    }
    return hitevent{.t = t,
                    .p = r.at(t),
                    .normal = normal,
                    .uv = vec2{0., 0.},
                    .m_id = s.material_id};
}

} // namespace cg
