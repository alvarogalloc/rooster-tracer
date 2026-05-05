export module camera;
import color_rgb;
import scene;
import vec3;
import std;
import ray;
export namespace cg {
struct camera {
  int width;
  int height;
  float fov;
  vec3 pos;
  vec3 up;
  vec3 lookAt;
  float far;
  float near;
  vec3 screen_to_ndc(int x, int y);
  ray compute_ray(int x, int y, vec3 forward, vec3 right, vec3 upVec);
  void cast_all_rays(std::function<void(ray, int, int)> ray_callback);
};

vec3 camera::screen_to_ndc(int x, int y) {
  if (height == 0)
    return vec3{0, 0, 0};
  const float aspectRatio = width / height;
  return vec3{
      (2 * ((x + 0.5f) / width) - 1) * std::tan(fov / 2) * aspectRatio,
      (1 - 2 * ((y + 0.5f) / height)) * std::tan(fov / 2),
      0,
  };
}
ray camera::compute_ray(int sx, int sy, vec3 forward, vec3 right, vec3 upVec) {
  const auto s_coord = screen_to_ndc(sx, sy);
  const auto dir =
      glm::normalize(forward + (right * s_coord.x) + (upVec * s_coord.y));
  return ray(pos, dir);
}

void camera::cast_all_rays(std::function<void(ray, int, int)> ray_callback) {
  const vec3 forward = glm::normalize(lookAt - pos);
  const vec3 right = glm::normalize(cross(forward, up));
  const vec3 upV = glm::normalize(cross(right, forward));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      // compute ray direction based on camera parameters
      const auto ray = compute_ray(x, y, forward, right, upV);
      ray_callback(ray, x, y);
    }
  }
}
} // namespace cg
