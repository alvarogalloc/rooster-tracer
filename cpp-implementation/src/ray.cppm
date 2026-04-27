export module ray;
import vec3;
export namespace cg
{
    struct ray
    {
        vec3 pos;
        vec3 dir;
        ray(vec3 pos, vec3 dir) : pos(pos), dir(cg::normalized(dir))
        {
        }
        auto at(const float t)
        {
            return pos + dir * t;
        }
    };
};
