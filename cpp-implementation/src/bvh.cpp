module bvh;
import aabb;
// great read for this:
// https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
namespace
{
using namespace cg;

constexpr float compute_sah_cost(bvh_node l, bvh_node r)
{
  // surface area heuristic: cost = left.count * left.area + right.count *
  // right.area used in subdivide() to pick the best split axis and position
  const auto surface_area = [](bvh_node n) {
    const auto extent = n.box_max - n.box_min;
    return 2 *
           (extent.x * extent.y + extent.y * extent.z + extent.x * extent.z);
  };
  return l.triangle_count * surface_area(l) +
         r.triangle_count * surface_area(r);
}

void subdivide(cg::bvh& b, std::span<const cg::triangle> tris,
               std::span<const glm::vec3> centroids, std::uint32_t node_index)
{
  constexpr static auto bucket_size{2};

  auto& node = b.nodes.at(node_index);
  if (node.triangle_count <= bucket_size)
    return;

  // find best split axis to make the division
  const glm::vec3 extent = node.box_max - node.box_min;
  const int axis = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2)
                                       : (extent.y > extent.z ? 1 : 2);

  const float split = (axis == 0   ? node.box_min.x + extent.x
                       : axis == 1 ? node.box_min.y + extent.y
                                   : node.box_min.z + extent.z) *
                      0.5f;

  const std::uint32_t first = node.left_child_or_first_index;
  const std::uint32_t count = node.triangle_count;

  auto index_span = std::span{b.tri_indices}.subspan(first, count);

  const auto mid_it = std::ranges::partition(index_span, [&](std::uint32_t i) {
    const glm::vec3& c = centroids[i];
    return (axis == 0 ? c.x : axis == 1 ? c.y : c.z) < split;
  });

  const std::uint32_t left_count = static_cast<std::uint32_t>(
      std::ranges::distance(index_span.begin(), mid_it.begin()));

  // degenerate split — all triangles ended up on one side
  if (left_count == 0 || left_count == count)
    return;

  const std::uint32_t left_index = static_cast<std::uint32_t>(b.nodes.size());

  const auto [left_min, left_max] = compute_span_aabb(
      tris, std::span{b.tri_indices}.subspan(first, left_count));
  const auto [right_min, right_max] = compute_span_aabb(
      tris,
      std::span{b.tri_indices}.subspan(first + left_count, count - left_count));

  b.nodes.emplace_back(left_min, first, left_max, left_count);
  b.nodes.emplace_back(right_min, first + left_count, right_max,
                       count - left_count);

  // turn current node into an interior node
  node.left_child_or_first_index = left_index;
  node.triangle_count = 0;

  subdivide(b, tris, centroids, left_index);
  subdivide(b, tris, centroids, left_index + 1);
}

} // namespace
namespace cg
{
void build_bvh(bvh& b, std::span<const triangle> mesh_tris)
{
  std::vector<vec3> centroids(mesh_tris.size());
  std::ranges::transform(mesh_tris, centroids.begin(), [](const triangle& t) {
    return (t.p0 + t.p1 + t.p2) / 3.f;
  });
  b.nodes.reserve(2 * mesh_tris.size() - 1);
  b.tri_indices.resize(mesh_tris.size());
  std::ranges::iota(b.tri_indices, 0);

  const auto [mesh_min, mesh_max] = compute_span_aabb(mesh_tris, b.tri_indices);
  b.nodes.emplace_back(mesh_min, 0, mesh_max,
                       static_cast<std::uint32_t>(mesh_tris.size()));
  subdivide(b, mesh_tris,centroids, 0);
}
} // namespace cg
