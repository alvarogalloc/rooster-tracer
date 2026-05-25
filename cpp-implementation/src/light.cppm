
export module light;
import std;
import color_rgb;
import directional_light;
import point_light;
import variant_overload;
export namespace cg
{

using light = std::variant<directional_light, point_light>;

constexpr inline auto shade_visitor = [](auto hit, auto material_shaded, auto view_direction) {
    return overload{[&](const directional_light& l) {
                        return shade_phong(material_shaded, hit, l, view_direction);
                    },
                    [&](const point_light& l) {
                        return shade_phong(material_shaded, hit, l, view_direction);
                    }

    };
};

using light_collection = std::vector<light>;
} // namespace cg
