export module color_rgb;
import vec3;
import std;

export using u32 = std::uint32_t;
export using u8 = std::uint8_t;

export namespace cg {
struct color_rgb : vec3 {
  using rgb255 = std::tuple<u8, u8, u8>;
  auto to_rgb_255() const {
    const float limit = 255.99f;

    // 3. Clamp values to prevent integer overflow on bright pixels
    float r = std::clamp(x, 0.0f, 1.0f);
    float g = std::clamp(y, 0.0f, 1.0f);
    float b = std::clamp(z, 0.0f, 1.0f);

    return rgb255{u8(r * limit), u8(g * limit), u8(b * limit)};
  }
  auto print() const {
    auto [r, g, b] = to_rgb_255();
    return std::println("{} {} {}", r, g, b);
  }
};
} // namespace cg
