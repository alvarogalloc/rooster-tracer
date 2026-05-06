export module object3d;
import color_rgb;
import hitevent;
import ray;
import std;
import interval;
export namespace cg
{
struct object3d
{
  std::size_t material_id{0};
  // virtual color_rgb color() const = 0;
  virtual std::optional<hitevent> get_hit(ray, interval) const = 0;
  virtual ~object3d() = default;
};
using object_collection = std::vector<std::unique_ptr<object3d>>;

} // namespace cg
