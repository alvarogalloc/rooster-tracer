export module bvh;
import std;
import glm;
import triangle;
export namespace cg
{
struct bvh_node
{
  // if triangle_count >0 then the node is a leaf and then
  // left_child_or_first_index is the first index
  //
  // if not, we are in an intermediate one, use left_child_or_first_index as
  // left and left_child_or_first_index+1 as right
  vec3 box_min;
  std::uint32_t left_child_or_first_index;
  vec3 box_max;
  std::uint32_t triangle_count;
  bool is_leaf() const noexcept
  {
    return triangle_count > 0;
  }
};

struct bvh
{
  std::vector<bvh_node> nodes{};
  std::vector<std::uint32_t> tri_indices{};
  std::uint32_t root_index{0};
};
// specifically 32 bits to fit two in single cache line
static_assert(sizeof(bvh_node) == 32, "bvh_node must be 32 bytes");

void build_bvh(bvh& mesh_bvh_info, std::span<const triangle> mesh_tris);

} // namespace cg
