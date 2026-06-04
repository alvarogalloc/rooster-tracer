module raytracer;
import std;

namespace cg
{
void render_scene(const scene& scene_data, std::span<color_rgb> image)
{
    const auto expected_pixel_count = static_cast<std::size_t>(
        scene_data.camera_data.width * scene_data.camera_data.height);
    if (image.size() != expected_pixel_count)
    {
        throw std::runtime_error{
            std::format("image buffer has {} pixels but expected {}",
                        image.size(), expected_pixel_count)};
    }

    const int height = scene_data.camera_data.height;
    const int width = scene_data.camera_data.width;
    const int max_depth = static_cast<int>(scene_data.max_depth);

    const vec3 forward = glm::normalize(scene_data.camera_data.lookAt - scene_data.camera_data.pos);
    const vec3 right = glm::normalize(cross(forward, scene_data.camera_data.up));
    const vec3 upV = glm::normalize(cross(right, forward));

    const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::jthread> threads;
    threads.reserve(num_threads);

    std::println("Rendering with {} threads...", num_threads);

    for (unsigned int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&, t]() {
            for (int y = static_cast<int>(t); y < height; y += num_threads)
            {
                for (int x = 0; x < width; ++x)
                {
                    const auto ray_data = scene_data.camera_data.compute_ray(x, y, forward, right, upV);
                    const auto index = static_cast<std::size_t>(width * y + x);
                    image[index] = trace_ray(scene_data, ray_data, max_depth);
                }
            }
        });
    }

    // Wait for all threads to finish
    threads.clear();
    std::println("Rendering complete.");
}
} // namespace cg
