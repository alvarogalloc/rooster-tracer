export module hitevent;
import glm;
import std;
export namespace cg
{
struct hitevent
{
    double t;
    vec3 p;
    vec3 normal;
    vec2 uv{0., 0.};
    vec3 dpdu{1., 0., 0.};
    vec3 dpdv{0., 1., 0.};
    std::size_t m_id{0};
};
} // namespace cg
