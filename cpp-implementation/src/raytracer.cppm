export module raytracer;
import camera;
import color_rgb;
import directional_light;
import material;
import stb;
import object3d;
import hitevent;
import scene;
import vec3;
import ray;
import interval;
import std;
export namespace cg
{
struct raytracer
{
  struct context
  {
    scene scene_;
    camera camera_;
    color_rgb bg_color;
    std::vector<color_rgb> image_;
    u32 maxDepth;
  };
  context ctx;

  raytracer(context&& ct) : ctx(std::move(ct))
  {
  }

  void run(std::string_view path)
  {
    render();
    saveImage(path);
  }

  void saveImage(std::string_view path)
  {
    const int w = ctx.camera_.width;
    const int h = ctx.camera_.height;
    const int channels = 3;

    std::vector<u8> pixels;
    pixels.reserve(w * h * channels);

    for (const color_rgb& c : ctx.image_)
    {
      auto [r, g, b] = c.to_rgb_255();
      pixels.push_back(r);
      pixels.push_back(g);
      pixels.push_back(b);
    }

    const int stride = w * channels;
    if (!stbi_write_png(path.data(), w, h, channels, pixels.data(), stride))
    {
      throw std::runtime_error(
          std::format("saveImage: failed to write '{}'", path));
    }
  }

  color_rgb trace_ray(ray ray, int depth)
  {
    if (depth <= 0)
    {
      return ctx.bg_color;
    }

    std::optional<hitevent> closest_hit;
    const interval i{ctx.camera_.near, ctx.camera_.far};
    for (const auto& obj : ctx.scene_.objects)
    {
      if (not bool(obj))
        continue;
      std::optional<hitevent> hit = obj->get_hit(ray, i);
      if (hit)
      {
        if (not closest_hit or (hit.value().t < closest_hit.value().t))
        {
          closest_hit = hit.value();
        }
      }
    }
    if (!closest_hit)
    {
      return ctx.bg_color;
    }

    const auto* sun =
        std::get_if<directional_light>(&ctx.scene_.lights.front());
    if (!sun)
    {
      throw std::runtime_error{"first light is not directional_light"};
    }

    const std::size_t material_id =
        closest_hit->m_id < ctx.scene_.materials.size() ? closest_hit->m_id : 0;
    return shade(ctx.scene_.materials.at(material_id), *closest_hit, *sun,
                 {0, 0, 0});
  }

  void render()
  {
    if (ctx.scene_.lights.empty())
    {
      std::println(std::cerr, "you should have at least one light!!");
      std::exit(-1);
    }
    if (ctx.scene_.materials.empty())
    {
      ctx.scene_.materials.emplace_back(material{color_rgb{1.f, 1.f, 1.f}});
    }
    ctx.camera_.cast_all_rays([this](auto ray, auto x, auto y) {
      color_rgb color = trace_ray(ray, ctx.maxDepth);
      // Set the color of the corresponding pixel in the image
      ctx.image_.at(ctx.camera_.width * y + x) = color;
    });
  }
};
} // namespace cg
