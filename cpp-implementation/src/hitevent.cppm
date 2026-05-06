export module hitevent;
import vec3;
import std;
export namespace cg
{
struct hitevent
{
  float t;
  vec3 p;
  vec3 normal;
  std::size_t m_id{0};
};
} // namespace cg
