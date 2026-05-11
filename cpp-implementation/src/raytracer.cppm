export module raytracer;
import camera;
import color_rgb;
import triangle;
import mesh3d;
import sphere;
import directional_light;
import material;
import stb;
import hitevent;
import scene;
import vec3;
import plane;
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
  template <class... Ts> struct overload : Ts...
  {
    using Ts::operator()...;
  };

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
      std::optional<hitevent> hit = [&] {
        return obj.visit(overload{
            [&](const triangle& tri) {
              return get_ray_triangle_hit(tri, ray, i);
            },
            [&](const sphere& sph) { return get_ray_sphere_hit(sph, ray, i); },
            [&](const mesh3d& mesh) {
              return get_ray_mesh_hit(mesh, ctx.scene_.mesh_triangles, ray, i);
            },
            [&](const plane& p) { return get_ray_plane_hit(p, ray, i); },
        });
      }();

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
    return shade_flat(ctx.scene_.materials.at(material_id), *closest_hit, *sun,
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
