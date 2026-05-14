
export module light;
import std;
import color_rgb;
import directional_light;
import point_light;
export namespace cg
{


using light = std::variant< directional_light, point_light>;
using light_collection = std::vector<light>;
} // namespace cg
