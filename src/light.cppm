
export module light;
import std;
import color_rgb;
import directional_light;
import point_light;
import spot_light;
import rect_light;
import variant_overload;
export namespace cg
{

using light = std::variant<directional_light, point_light, spot_light, rect_light>;
auto shade_visitor(const auto& hit, const auto& material_shaded,
                   const auto& view_direction)
{
    return overload{
        [&](const directional_light& l) {
            return shade_phong(material_shaded, hit, l, view_direction);
        },
        [&](const point_light& l) {
            return shade_phong(material_shaded, hit, l, view_direction);
        },
        [&](const spot_light& l) {
            return shade_phong(material_shaded, hit, l, view_direction);
        },
        [&](const rect_light& l) {
            return color_rgb{0.0, 0.0, 0.0};
        },
    };
}

using light_collection = std::vector<light>;
} // namespace cg
