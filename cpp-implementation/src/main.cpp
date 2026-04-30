import std;
import scene;
import color_rgb;
import raytracer;
import sphere;
import vec3;
import camera;
import triangle;
import scene_parser;
int main(int argc, char **argv) {
  using namespace cg;
  // auto cam = camera{
  //     .width = 400,
  //     .height = 400,
  //     .fov = std::numbers::pi_v<float> / 2,
  //     .pos = vec3{0, 0, 0},
  //     .up = vec3{0, 1, 0},
  //     .lookAt = vec3{0, 0, -1},
  //     .far = 1000.f,
  //     .near = 0.01f,
  // };
auto cam = camera{
    .width  = 400,
    .height = 400,
    .fov    = std::numbers::pi_v<float> / 4, // 45° — tight enough to fill frame
    .pos    = vec3{ 0.4f, 0.65f, 1.4f },     // elevated, slightly to the right, pulled back
    .up     = vec3{ 0.f,  1.f,   0.f  },
    .lookAt = vec3{ -0.037f, 0.458f, 0.192f }, // exact centroid
    .far    = 20.f,
    .near   = 0.001f,
};

  if (argc != 3) {
    std::println(std::cerr,
                 "please set the scene to render and the output img\n usage: "
                 "rooster-tracer scene.rscn output.png");
    return -1;
  }
  const auto scene_path{argv[1]};
  const auto img_path{argv[2]};
  const auto n_pixels = static_cast<std::size_t>(cam.width * cam.height);
  try {
    auto rt = raytracer{
        raytracer::context{.scene_ = {cg::parse_scene(scene_path)},
                           .camera_ = cam,
                           .bg_color = color_rgb::from_rgb_256(10,32,90),
                           .image_{n_pixels},
                           .maxDepth = 5},
    };
    rt.run(img_path);
  } catch (const std::exception &ex) {
    std::println(std::cerr, "exception caught (rooster-tracer)!, exiting...\n[reason] -> \"{}\"", ex.what());
    return -1;
  }
  std::println("{} rendered correctly!!!", img_path);
}
