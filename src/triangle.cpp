module triangle;
import glm;
import interval;

namespace cg
{

std::optional<hitevent> get_ray_triangle_hit(const triangle& tt,
                                             std::span<const vertex> vertices,
                                             ray r, interval i,
                                             bool cull_backfaces)
{
    const vertex& v0 = vertices[tt.vertex_start];
    constexpr double epsilon = 1e-8f;

    const vertex& v1 = vertices[tt.vertex_start + 1];
    const vertex& v2 = vertices[tt.vertex_start + 2];
    const vec3& p0 = v0.p;
    const vec3& p1 = v1.p;
    const vec3& p2 = v2.p;
    const vec3& n0 = v0.n;
    const vec3& n1 = v1.n;
    const vec3& n2 = v2.n;
    // const vec3& p2 = tt.v3.p;

    using glm::mat3;
    const auto col0 = -r.dir;
    const auto col1 = p1 - p0;
    const auto col2 = p2 - p0;
    const auto col3 = r.pos - p0;

    const auto d0 = glm::determinant(mat3{col0, col1, col2});
    const interval close_to_zero{-1e-4, 1e-4};
    if (close_to_zero.contains(d0)) // paralelo al triangulo
        return std::nullopt;

    if (d0 < 0. && cull_backfaces)
        return std::nullopt;

    const auto dt = glm::determinant(mat3{col3, col1, col2});
    const double t = dt / d0;
    if (t < 0. || !i.contains(t)) // esta detras del rayo
        return std::nullopt;

    const auto du = glm::determinant(mat3{col0, col3, col2});
    const interval zero_to_one{0., 1.};
    const double u = du / d0;
    if (!zero_to_one.contains(u))
        return std::nullopt;

    const auto dv = glm::determinant(mat3{col0, col1, col3});
    const interval v_range{0., 1. - u};
    const double bary_v = dv / d0;
    if (!v_range.contains(bary_v))
        return std::nullopt;

    hitevent hit;
    hit.t = t;
    hit.p = r.at(t);
    const double bary_u = u;
    const double bary_w = 1. - bary_u - bary_v;
    const vec3 face_normal = safe_normalize(glm::cross(col1, col2));

    // compute normal
    vec3 normal =
        safe_normalize(n0 * bary_w + n1 * bary_u + n2 * bary_v, epsilon);
    if (glm::dot(normal, normal) <= epsilon)
        normal = face_normal;
    if (glm::dot(normal, r.dir) > 0.)
    {
        normal = -normal;
    }
    hit.normal = vec3{normal.x, normal.y, normal.z};

    hit.m_id = tt.material_id;
    // compute_uv
    hit.uv = v0.uv * bary_w + v1.uv * bary_u + v2.uv * bary_v;

    const vec2 duv1 = v1.uv - v0.uv;
    const vec2 duv2 = v2.uv - v0.uv;
    const double det = duv1.x * duv2.y - duv1.y * duv2.x;
    if (std::abs(det) > epsilon)
    {
        const double inv_det = 1.0 / det;
        hit.dpdu = (col1 * duv2.y - col2 * duv1.y) * inv_det;
        hit.dpdv = (col2 * duv1.x - col1 * duv2.x) * inv_det;
    }
    else
    {
        // fallback if UVs are degenerate
        vec3 a;
        if (std::abs(normal.x) > 0.9)
            a = vec3(0, 1, 0);
        else
            a = vec3(1, 0, 0);
        hit.dpdu = safe_normalize(glm::cross(normal, a), epsilon);
        hit.dpdv = safe_normalize(glm::cross(normal, hit.dpdu), epsilon);
    }
    return hit;
}

} // namespace cg
