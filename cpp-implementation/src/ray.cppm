export module ray;
import vec3;
import glm;
export namespace cg
{
struct ray
{
  vec3 pos;
  vec3 dir;
  ray(vec3 pos, vec3 dir) : pos(pos), dir(glm::normalize(dir))
  {
  }
  auto at(const float t)
  {
    return pos + dir * t;
  }
};
}; // namespace cg
