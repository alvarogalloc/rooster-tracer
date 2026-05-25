export module ray;
import glm;
import glm;
export namespace cg
{
struct ray
{
    vec3 pos;
    vec3 dir;
    constexpr ray(vec3 pos, vec3 dir) noexcept
        : pos(pos), dir(glm::normalize(dir))
    {
    }
    auto at(const double t)
    {
        return pos + dir * t;
    }
};
}; // namespace cg
