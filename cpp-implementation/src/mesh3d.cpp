module mesh3d;
import triangle;
namespace cg
{

std::optional<cg::hitevent> get_ray_mesh_hit(const mesh3d& mesh,
                                             std::span<const triangle> tris,
                                             ray r, interval valid)
{
  if (mesh.blas.nodes.empty())
    return std::nullopt;

  std::optional<hitevent> result{};

  // explicit stack — fixed size, no heap allocation on the hot path
  std::array<std::uint32_t, 64> stack{};
  std::uint32_t stack_top{0};
  std::uint32_t current{mesh.blas.root_index};

  while (true)
  {
    const bvh_node& node = mesh.blas.nodes[current];

    if (node.is_leaf())
    {
      // test every triangle in this leaf
      for (std::uint32_t i = 0; i < node.triangle_count; ++i)
      {
        const std::uint32_t tri_index =
            mesh.blas.tri_indices[node.left_child_or_first_index + i];

        if (auto hit = get_ray_triangle_hit(tris[tri_index], r, valid))
        {
          valid.max = hit->t; // narrow the interval — discard farther hits
          result = hit;
        }
      }

      // pop next node from stack or finish
      if (stack_top == 0)
        break;
      current = stack[--stack_top];
    }
    else
    {
      // interior node — test both children, push farther one first
      const std::uint32_t left = node.left_child_or_first_index;
      const std::uint32_t right = left + 1;

      const auto aabb_l =
          aabb{mesh.blas.nodes[left].box_min, mesh.blas.nodes[left].box_max};
      const auto aabb_r =
          aabb{mesh.blas.nodes[right].box_min, mesh.blas.nodes[right].box_max};
      const bool hit_left = is_ray_aabb_hit(aabb_l, r, valid);
      const bool hit_right = is_ray_aabb_hit(aabb_r, r, valid);

      if (!hit_left && !hit_right)
      {
        if (stack_top == 0)
          break;
        current = stack[--stack_top];
      }
      else if (hit_left && hit_right)
      {
        // push farther, traverse nearer — reduces nodes visited
        // for now just push right and go left
        stack[stack_top++] = right;
        current = left;
      }
      else
      {
        current = hit_left ? left : right;
      }
    }
  }

  if (result)
  {
    result.value().m_id = mesh.material_id;
  }
  return result;
}
} // namespace cg
