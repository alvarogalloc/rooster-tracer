module bvh;
import aabb;
import glm;
// great read for this:
// https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
namespace
{
using namespace cg;

constexpr std::uint32_t kMaxLeafTriangles{2};
constexpr std::uint32_t kBinCount{16};

[[nodiscard]] aabb empty_aabb()
{
    const double inf = std::numeric_limits<double>::infinity();
    return aabb{.min = {inf, inf, inf}, .max = {-inf, -inf, -inf}};
}

[[nodiscard]] bool is_empty(const aabb& box)
{
    return box.min.x > box.max.x;
}

void extend(aabb& box, const vec3& p)
{
    box.min = glm::min(box.min, p);
    box.max = glm::max(box.max, p);
}

void extend(aabb& box, const aabb& other)
{
    if (is_empty(other))
        return;
    if (is_empty(box))
    {
        box = other;
        return;
    }
    box.min = glm::min(box.min, other.min);
    box.max = glm::max(box.max, other.max);
}

[[nodiscard]] aabb triangle_bounds(const vec3& p0, const vec3& p1,
                                   const vec3& p2)
{
    return aabb{.min = glm::min(p0, glm::min(p1, p2)),
                .max = glm::max(p0, glm::max(p1, p2))};
}

[[nodiscard]] double surface_area(const aabb& box)
{
    const vec3 e = box.max - box.min;
    return 2.0 * (e.x * e.y + e.x * e.z + e.y * e.z);
}

[[nodiscard]] double axis_value(const vec3& v, int axis)
{
    return axis == 0 ? v.x : axis == 1 ? v.y : v.z;
}

[[nodiscard]] int bin_index(double c, double axis_min, double inv_bin_size)
{
    return std::clamp(static_cast<int>((c - axis_min) * inv_bin_size), 0,
                      static_cast<int>(kBinCount) - 1);
}

struct bin
{
    aabb bounds{empty_aabb()};
    std::uint32_t count{0};
};

struct bin_sums
{
    std::array<aabb, kBinCount> bounds{};
    std::array<std::uint32_t, kBinCount> counts{};
};

struct sah_split
{
    int axis;
    std::uint32_t split_bin;
    std::uint32_t left_count;
    std::uint32_t right_count;
    aabb left_bounds;
    aabb right_bounds;
    double cost;
};

[[nodiscard]] std::array<bin, kBinCount> build_bins(
    std::span<const std::uint32_t> indices, std::span<const vec3> centroids,
    std::span<const aabb> tri_bounds, int axis, double axis_min,
    double inv_bin_size)
{
    std::array<bin, kBinCount> bins{};
    for (const std::uint32_t tri_index : indices)
    {
        const double c = axis_value(centroids[tri_index], axis);
        const int index = bin_index(c, axis_min, inv_bin_size);
        bins[index].count += 1;
        extend(bins[index].bounds, tri_bounds[tri_index]);
    }
    return bins;
}

[[nodiscard]] bin_sums prefix_sums(const std::array<bin, kBinCount>& bins)
{
    bin_sums sums{};
    aabb running_bounds = empty_aabb();
    std::uint32_t running_count = 0;
    for (std::uint32_t i = 0; i < kBinCount; ++i)
    {
        running_count += bins[i].count;
        sums.counts[i] = running_count;
        extend(running_bounds, bins[i].bounds);
        sums.bounds[i] = running_bounds;
    }
    return sums;
}

[[nodiscard]] bin_sums suffix_sums(const std::array<bin, kBinCount>& bins)
{
    bin_sums sums{};
    aabb running_bounds = empty_aabb();
    std::uint32_t running_count = 0;
    for (std::uint32_t i = kBinCount; i-- > 0;)
    {
        running_count += bins[i].count;
        sums.counts[i] = running_count;
        extend(running_bounds, bins[i].bounds);
        sums.bounds[i] = running_bounds;
    }
    return sums;
}

// esto es lo bueno
//
[[nodiscard]] std::optional<sah_split> best_axis_split(
    std::span<const std::uint32_t> indices, std::span<const vec3> centroids,
    std::span<const aabb> tri_bounds, const aabb& centroid_bounds, int axis)
{
    const vec3 extent = centroid_bounds.max - centroid_bounds.min;
    const double axis_extent = axis_value(extent, axis);
    if (axis_extent <= std::numeric_limits<double>::epsilon())
        return std::nullopt;

    const double axis_min = axis_value(centroid_bounds.min, axis);
    const double inv_bin_size = static_cast<double>(kBinCount) / axis_extent;
    const auto bins = build_bins(indices, centroids, tri_bounds, axis, axis_min,
                                 inv_bin_size);
    const auto left = prefix_sums(bins);
    const auto right = suffix_sums(bins);

    std::optional<sah_split> best{};
    double best_cost = std::numeric_limits<double>::infinity();
    for (std::uint32_t split = 0; split + 1 < kBinCount; ++split)
    {
        if (left.counts[split] == 0 || right.counts[split + 1] == 0)
            continue;

        const double cost =
            left.counts[split] * surface_area(left.bounds[split]) +
            right.counts[split + 1] * surface_area(right.bounds[split + 1]);
        if (cost < best_cost)
        {
            best_cost = cost;
            best = sah_split{.axis = axis,
                             .split_bin = split,
                             .left_count = left.counts[split],
                             .right_count = right.counts[split + 1],
                             .left_bounds = left.bounds[split],
                             .right_bounds = right.bounds[split + 1],
                             .cost = cost};
        }
    }
    return best;
}

[[nodiscard]] std::optional<sah_split> find_best_split(
    std::span<const std::uint32_t> indices, std::span<const vec3> centroids,
    std::span<const aabb> tri_bounds, const aabb& centroid_bounds)
{
    std::optional<sah_split> best{};
    for (int axis = 0; axis < 3; ++axis)
    {
        auto candidate = best_axis_split(indices, centroids, tri_bounds,
                                         centroid_bounds, axis);
        if (!candidate)
            continue;
        // minimizar coste
        if (!best || candidate->cost < best->cost)
            best = std::move(candidate);
    }
    return best;
}

// dado una coleccion de centroides sacar su caja
[[nodiscard]] aabb centroid_bounds_for(std::span<const std::uint32_t> indices,
                                       std::span<const vec3> centroids)
{
    aabb bounds = empty_aabb();
    for (const std::uint32_t tri_index : indices)
    {
        extend(bounds, centroids[tri_index]);
    }
    return bounds;
}

[[nodiscard]] bool is_degenerate(const aabb& bounds)
{
    const vec3 extent = bounds.max - bounds.min;
    return extent.x <= 0. && extent.y <= 0. && extent.z <= 0.;
}

[[nodiscard]] std::uint32_t partition_by_split(std::span<std::uint32_t> indices,
                                               std::span<const vec3> centroids,
                                               const aabb& centroid_bounds,
                                               const sah_split& split)
{
    const vec3 extent = centroid_bounds.max - centroid_bounds.min;
    const double axis_extent = axis_value(extent, split.axis);
    if (axis_extent <= std::numeric_limits<double>::epsilon())
        return 0;

    const double axis_min = axis_value(centroid_bounds.min, split.axis);
    const double inv_bin_size = static_cast<double>(kBinCount) / axis_extent;
    const auto mid_it = std::ranges::partition(indices, [&](std::uint32_t i) {
        const double c = axis_value(centroids[i], split.axis);
        return static_cast<std::uint32_t>(
                   bin_index(c, axis_min, inv_bin_size)) <= split.split_bin;
    });
    return static_cast<std::uint32_t>(
        std::ranges::distance(indices.begin(), mid_it.begin()));
}

void create_children(bvh& b, const sah_split& split, std::uint32_t first,
                     std::uint32_t left_count, std::uint32_t total_count,
                     std::uint32_t& left_index)
{
    left_index = static_cast<std::uint32_t>(b.nodes.size());
    b.nodes.emplace_back(split.left_bounds.min, first, split.left_bounds.max,
                         left_count);
    b.nodes.emplace_back(split.right_bounds.min, first + left_count,
                         split.right_bounds.max, total_count - left_count);
}

/*
 * aqui inicio en node_index y inicio creacion de nuevos nodos hijos
 */
void subdivide(cg::bvh& b, std::span<const vec3> centroids,
               std::span<const aabb> tri_bounds, std::uint32_t node_index)
{
    auto& node = b.nodes.at(node_index);
    // salimos si somos una caja que podemos recorrer en O(n), porque tenemos
    // maximo kMaxLeafTriangles como n
    if (node.triangle_count <= kMaxLeafTriangles)
        return;

    // somos una caja intermedia, entonces left_child_or_first_index es el index
    // para la caja izq.
    const std::uint32_t first = node.left_child_or_first_index;
    const std::uint32_t count = node.triangle_count;
    auto index_span = std::span{b.tri_indices}.subspan(first, count);

    // le pasamos un subconjunto de triangulos y nos da su caja
    const aabb centroid_bounds = centroid_bounds_for(index_span, centroids);
    // no puede ser de area <= 0
    if (is_degenerate(centroid_bounds))
        return;

    // el mejor corte sera el que la suma de las areas de sus triangulos
    // contenidos sea la maxima, por lo que tenemos mas probabilidad de darle
    const auto split =
        find_best_split(index_span, centroids, tri_bounds, centroid_bounds);
    if (!split)
        return;

    const std::uint32_t left_count =
        partition_by_split(index_span, centroids, centroid_bounds, *split);
    if (left_count == 0 || left_count == count)
        return;

    std::uint32_t left_index = 0;
    create_children(b, *split, first, left_count, count, left_index);

    // re-fetch node reference because create_children might have reallocated
    // the vector
    auto& interior_node = b.nodes.at(node_index);
    interior_node.left_child_or_first_index = left_index;
    interior_node.triangle_count = 0;

    subdivide(b, centroids, tri_bounds, left_index);
    subdivide(b, centroids, tri_bounds, left_index + 1);
}

// agarrar el centroide de cada triangulo y su caja aabb
void fill_triangle_data(std::span<const triangle> mesh_tris,
                        std::span<const vertex> vertices,
                        std::span<vec3> centroids, std::span<aabb> tri_bounds)
{
    for (std::size_t i = 0; i < mesh_tris.size(); ++i)
    {
        const triangle& tri = mesh_tris[i];
        const vec3& p0 = vertices[tri.vertex_start].p;
        const vec3& p1 = vertices[tri.vertex_start + 1].p;
        const vec3& p2 = vertices[tri.vertex_start + 2].p;

        centroids[i] = (p0 + p1 + p2) / 3.;
        tri_bounds[i] = triangle_bounds(p0, p1, p2);
    }
}

[[nodiscard]] aabb merge_bounds(std::span<const aabb> bounds)
{
    aabb result = empty_aabb();
    for (const auto& box : bounds)
    {
        extend(result, box);
    }
    return result;
}

} // namespace
namespace cg
{
void build_bvh(bvh& b, std::span<const triangle> mesh_tris,
               std::span<const vertex> vertices)
{
    b.nodes.clear();
    b.tri_indices.clear();
    if (mesh_tris.empty())
        return;

    std::vector<vec3> centroids(mesh_tris.size());
    std::vector<aabb> tri_bounds(mesh_tris.size());

    fill_triangle_data(mesh_tris, vertices, centroids, tri_bounds);
    b.nodes.reserve(2 * mesh_tris.size() - 1);
    b.tri_indices.resize(mesh_tris.size());
    // al principio, todos los vertices estan en una caja, del 0..n
    // despues se cambiaran para especificar por cajas
    // ya que cada nodo va a referenciar un inicio de vertices
    std::ranges::iota(b.tri_indices, 0);

    // la caja del mesh es hecha con el min y max de las cajas de los triangulos
    const aabb mesh_bounds = merge_bounds(tri_bounds);

    // primer caja es la que intersecta a todo el mesh, es el inicio del
    // recorrido
    b.nodes.emplace_back(mesh_bounds.min, 0, mesh_bounds.max,
                         static_cast<std::uint32_t>(mesh_tris.size()));

    // hacer mas cajas
    subdivide(b, centroids, tri_bounds, 0);
}
} // namespace cg
