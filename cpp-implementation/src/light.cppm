
export module light;
import std;
import color_rgb;
import directional_light;
export namespace cg
{

struct dummy_light
{
  [[nodiscard]] auto radiance() const
  {
    return color_rgb{};
  }
};

using light = std::variant<dummy_light, directional_light>;
using light_collection = std::vector<light>;
} // namespace cg
