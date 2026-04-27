export module color_rgb;
import vec3;
import std;

export using u32 = std::uint32_t;
export using u8 = std::uint8_t;

export namespace cg {
struct color_rgb : vec3 {
  using rgba255 = std::tuple<u8, u8, u8>;
  auto to_rgb_255() const {
    const float limit = 255.99f;
    const auto [r, g, b] = (*this) * limit;
    return rgba255{u8(r), u8(g), u8(b)};
  }
  auto print() const {
    auto [r, g, b] = to_rgb_255();
    return std::println("{} {} {}", r, g, b);
  }
};
} // namespace cg
