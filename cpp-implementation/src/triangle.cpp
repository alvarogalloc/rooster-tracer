module triangle;
import glm;
import interval;

namespace cg
{
namespace
{
constexpr float kEpsLen2 = 1e-12f;

[[nodiscard]] vec3 safe_normalize(vec3 v)
{
    const float len_sq = glm::dot(v, v);
    if (len_sq <= kEpsLen2)
        return vec3{0.f, 0.f, 0.f};
    return v * glm::inversesqrt(len_sq);
}
} // namespace

std::optional<hitevent> get_ray_triangle_hit(const triangle& tt,
                                             std::span<const vertex> vertices,
                                             ray r, interval i,
                                             bool cull_backfaces)
{
    const vertex& v0 = vertices[tt.vertex_start];
    const vertex& v1 = vertices[tt.vertex_start + 1];
    const vertex& v2 = vertices[tt.vertex_start + 2];
    const vec3& p0 = v0.p;
    const vec3& p1 = v1.p;
    const vec3& p2 = v2.p;
    const vec3& n0 = v0.n;
    const vec3& n1 = v1.n;
    const vec3& n2 = v2.n;
    const bool has_vertex_normals =
        v0.has_normal && v1.has_normal && v2.has_normal;
    const bool has_vertex_uvs = v0.has_uv && v1.has_uv && v2.has_uv;
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

    if (d0 < 0.f && cull_backfaces)
        return std::nullopt;

    const auto dt = glm::determinant(mat3{col3, col1, col2});
    const float t = dt / d0;
    if (t < 0.f || !i.contains(t)) // esta detras del rayo
        return std::nullopt;

    const auto du = glm::determinant(mat3{col0, col3, col2});
    const interval zero_to_one{0.f, 1.f};
    const float u = du / d0;
    if (!zero_to_one.contains(u))
        return std::nullopt;

    const auto dv = glm::determinant(mat3{col0, col1, col3});
    const interval v_range{0.f, 1.f - u};
    const float bary_v = dv / d0;
    if (!v_range.contains(bary_v))
        return std::nullopt;

    hitevent hit;
    hit.t = t;
    hit.p = r.at(t);
    const float bary_u = u;
    const float bary_w = 1.f - bary_u - bary_v;
    const vec3 face_normal = safe_normalize(glm::cross(col1, col2));
    vec3 normal = face_normal;
    if (has_vertex_normals)
    {
        normal = safe_normalize(n0 * bary_w + n1 * bary_u + n2 * bary_v);
        if (glm::dot(normal, normal) <= kEpsLen2)
            normal = face_normal;
    }
    if (glm::dot(normal, r.dir) > 0.f)
    {
        normal = -normal;
    }
    hit.normal = vec3{normal.x, normal.y, normal.z};
    if (has_vertex_uvs)
    {
        hit.uv = v0.uv * bary_w + v1.uv * bary_u + v2.uv * bary_v;

        const vec2 duv1 = v1.uv - v0.uv;
        const vec2 duv2 = v2.uv - v0.uv;
        const float det = duv1.x * duv2.y - duv1.y * duv2.x;
        if (std::abs(det) > 1e-8f)
        {
            const float inv_det = 1.0f / det;
            hit.dpdu = (col1 * duv2.y - col2 * duv1.y) * inv_det;
            hit.dpdv = (col2 * duv1.x - col1 * duv2.x) * inv_det;
        }
        else
        {
            // fallback if UVs are degenerate
            vec3 a;
            if (std::abs(normal.x) > 0.9f)
                a = vec3(0, 1, 0);
            else
                a = vec3(1, 0, 0);
            hit.dpdu = safe_normalize(glm::cross(normal, a));
            hit.dpdv = safe_normalize(glm::cross(normal, hit.dpdu));
        }
    }
    hit.m_id = tt.material_id;
    return hit;
}

} // namespace cg
