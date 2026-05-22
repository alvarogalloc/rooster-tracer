export module hitevent;
import glm;
import std;
export namespace cg
{
struct hitevent
{
    float t;
    vec3 p;
    vec3 normal;
    vec2 uv{0.f, 0.f};
    vec3 dpdu{1.f, 0.f, 0.f};
    vec3 dpdv{0.f, 1.f, 0.f};
    std::size_t m_id{0};
};
} // namespace cg
