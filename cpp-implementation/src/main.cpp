import std;
import scene;
import color_rgb;
import raytracer;
import sphere;
import vec3;
import camera;
int main() {
  using namespace cg;
  auto cam = camera{
      .width = 400,
      .height = 400,
      .fov = std::numbers::pi_v<float> / 2,
      .pos = vec3{0, 0, 0},
      .up = vec3{0, 1, 0},
      .lookAt = vec3{0, 0, -1},
  };

  // auto s = scene{};

  auto rt = raytracer{raytracer::context{
      .scene = cg::scene{},
      .camera = cam,
      .bg_color = color_rgb{1, 1, 1},
      .image = std::vector<color_rgb>(cam.width * cam.height),
      .maxDepth = 5}};
  rt.ctx.scene.objects.push_back(std::make_unique<sphere>(1.f,
                                                          vec3{
                                                              0,
                                                              0,
                                                              2,
                                                          },
                                                          color_rgb{0, 0, 0.5}));
  rt.run("outimage.png");
}
