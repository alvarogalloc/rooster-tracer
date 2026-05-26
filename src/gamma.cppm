export module gamma;
import std;
import glm;
// gamma correction constants
// sRGB to linear conversion constants (IEC 61966-2-1)
constexpr double k_linear_thresh = 0.04045;
constexpr double k_linear_scale = 12.92;
constexpr double k_gamma_offset = 0.055;
constexpr double k_gamma_scale = 1.055;
constexpr double k_gamma_exp = 2.4;
constexpr double k_linear_thresh_inv = 0.0031308;
export namespace cg
{

// sRGB channel to linear light
[[nodiscard]] constexpr double srgb_to_linear(double v) noexcept
{
    if (v <= k_linear_thresh)
        return v / k_linear_scale;
    return std::pow((v + k_gamma_offset) / k_gamma_scale, k_gamma_exp);
}

[[nodiscard]] constexpr vec3 srgb_to_linear(vec3 v) noexcept
{
    return {srgb_to_linear(v.x), srgb_to_linear(v.y), srgb_to_linear(v.z)};
}

// linear light channel to sRGB
[[nodiscard]] constexpr double linear_to_srgb(double v) noexcept
{
    v = std::clamp(v, 0.0, 1.0);
    if (v <= k_linear_thresh_inv)
        return v * k_linear_scale;
    return k_gamma_scale * std::pow(v, 1.0 / k_gamma_exp) - k_gamma_offset;
}

[[nodiscard]] constexpr vec3 linear_to_srgb(vec3 v) noexcept
{
    return {linear_to_srgb(v.x), linear_to_srgb(v.y), linear_to_srgb(v.z)};
}
} // namespace cg
