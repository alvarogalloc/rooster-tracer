
export module aabb;
import std;
import hitevent;
import interval;
import triangle;
import ray;
import glm;

export namespace cg
{

struct aabb
{
    vec3 min;
    vec3 max;
};

bool is_ray_aabb_hit(const aabb&, ray, interval);
} // namespace cg
