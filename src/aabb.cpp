module aabb;
import std;
import hitevent;
import interval;
import ray;
import glm;

namespace cg
{

bool is_ray_aabb_hit(const aabb& box, ray r, interval valid)
{
    // planes and boxes are related
    // the boxes can be represented a many planes in a given xy xz or yz domain
    // there are two intersections and we are going to take the closest one,
    // i.e. min t we can say that the ray entered the box if it intersects any
    // two different planes
    const vec3 inv_dir = 1.0 / r.dir;

    const vec3 t_near = (box.min - r.pos) * inv_dir;
    const vec3 t_far = (box.max - r.pos) * inv_dir;

    const vec3 t_min = glm::min(t_near, t_far);
    const vec3 t_max = glm::max(t_near, t_far);

    const double t_enter = glm::compMax(t_min);
    const double t_exit = glm::compMin(t_max);

    const double t_start = std::max(t_enter, valid.min);
    const double t_end = std::min(t_exit, valid.max);

    return t_start <= t_end;
}
} // namespace cg
