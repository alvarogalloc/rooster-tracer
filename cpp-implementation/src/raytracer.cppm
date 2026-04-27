export module raytracer;
import camera;
import color_rgb;
import stb;
import object3d;
import hitevent;
import scene;
import vec3;
import ray;
import std;
export namespace cg {
struct raytracer {

  struct context {
    scene scene;
    camera camera;
    color_rgb bg_color;
    std::vector<color_rgb> image;
    u32 maxDepth;
  };
  context ctx;

  raytracer(context &&ct) : ctx(std::move(ct)) {}

  void run(std::string_view path) {
    render();
    saveImage(path);
  }

  void saveImage(std::string_view path) {
    const int w = ctx.camera.width;
    const int h = ctx.camera.height;
    const int channels = 3;

    std::vector<u8> pixels;
    pixels.reserve(w * h * channels);

    for (const color_rgb &c : ctx.image) {
      auto [r, g, b] = c.to_rgb_255();
      pixels.push_back(r);
      pixels.push_back(g);
      pixels.push_back(b);
    }

    const int stride = w * channels;
    if (!stbi_write_png(path.data(), w, h, channels, pixels.data(), stride)) {
      throw std::runtime_error(
          std::format("saveImage: failed to write '{}'", path));
    }
  }

  color_rgb trace_ray(ray ray, int depth) {
    if (depth <= 0) {
      return ctx.bg_color;
    }
    std::optional<hitevent> closest_hit;
    std::optional<object3d *> hit_obj;
    for (const auto &obj : ctx.scene.objects) {
      if (not bool(obj))
        continue;
      std::optional<hitevent> hit = obj->get_hit(ray);
      if (hit) {
        if (not closest_hit or (hit.value().t < closest_hit.value().t)) {
          closest_hit = hit.value();
          hit_obj = obj.get();
        }
      }
    }
    if (closest_hit and hit_obj) {
      return hit_obj.value()->color();
    } else {
      return ctx.bg_color;
    }
  }

  void render() {
    ctx.camera.cast_all_rays([this](auto ray, auto x, auto y) {
      color_rgb color = trace_ray(ray, ctx.maxDepth);
      // Set the color of the corresponding pixel in the image
      ctx.image.at(ctx.camera.width * y + x) = color;
    });
  }
};
} // namespace cg
