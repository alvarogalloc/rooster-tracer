export module camera;
import glm;
import std;
import ray;
export namespace cg
{
struct camera
{
    int width{1280};
    int height{720};
    double fov{1.0472};
    vec3 pos{0., 2., 5.};
    vec3 up{0., 1., 0.};
    vec3 lookAt{0., -1., -4.};
    double far{20.};
    double near{0.001};
    vec3 screen_to_ndc(int x, int y) const;
    ray compute_ray(int x, int y, vec3 forward, vec3 right, vec3 upVec) const;
    void cast_all_rays(std::function<void(ray, int, int)> ray_callback) const;
};

vec3 camera::screen_to_ndc(int x, int y) const
{
    if (height == 0)
        return vec3{0, 0, 0};
    const double aspectRatio =
        static_cast<double>(width) / static_cast<double>(height);
    return vec3{
        (2 * ((x + 0.5) / width) - 1) * std::tan(fov / 2) * aspectRatio,
        (1 - 2 * ((y + 0.5) / height)) * std::tan(fov / 2),
        0,
    };
}
ray camera::compute_ray(int sx, int sy, vec3 forward, vec3 right,
                        vec3 upVec) const
{
    const auto s_coord = screen_to_ndc(sx, sy);
    const auto dir =
        glm::normalize(forward + (right * s_coord.x) + (upVec * s_coord.y));
    return ray(pos, dir);
}

void camera::cast_all_rays(
    std::function<void(ray, int, int)> ray_callback) const
{
    const vec3 forward = glm::normalize(lookAt - pos);
    const vec3 right = glm::normalize(cross(forward, up));
    const vec3 upV = glm::normalize(cross(right, forward));

    const auto totalPixelCount = height * width;
    int pixelsDone = 0;
    int lastReported = -1;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const auto ray = compute_ray(x, y, forward, right, upV);
            ray_callback(ray, x, y);

            const int pct = (++pixelsDone * 100) / totalPixelCount;
            const int milestone = pct / 10;
            if (milestone != lastReported)
            {
                lastReported = milestone;
                std::print("\rRendering... {}%", milestone * 10);
                std::cout.flush();
            }
        }
    }
    std::println(""); // newline after final 100%
}
} // namespace cg
