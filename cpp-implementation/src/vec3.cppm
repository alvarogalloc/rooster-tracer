export module vec3;
import std;
export namespace cg {
struct vec3 {
  float x;
  float y;
  float z;
  constexpr auto span() { return std::span<float, 3>{&x, 3}; }

  auto print() const { std::println("x:{} y:{} z:{}", x, y, z); }
  constexpr auto &operator[](const std::uint8_t index) { return span()[index]; }
  constexpr auto &operator+=(const vec3 &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  constexpr auto &operator*=(float t) {
    x *= t;
    y *= t;
    z *= t;
    return *this;
  }
  constexpr auto &operator/=(float t) { return *this *= 1 / t; }

  constexpr auto length_squared() const { return x * x + y * y + z * z; }
   auto length() const { return std::sqrt(length_squared()); }
};

std::ostream &operator<<(std::ostream &out, const vec3 &v) {
  return out << v.x << ' ' << v.y << ' ' << v.z;
}

constexpr vec3 operator+(const vec3 &u, const vec3 &v) {
  return vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

constexpr vec3 operator-(const vec3 &u, const vec3 &v) {
  return vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

constexpr vec3 operator*(const vec3 &u, const vec3 &v) {
  return vec3(u.x * v.x, u.y * v.y, u.z * v.z);
}

constexpr vec3 operator*(float t, const vec3 &v) {
  return vec3(t * v.x, t * v.y, t * v.z);
}

constexpr vec3 operator*(const vec3 &v, float t) { return t * v; }

constexpr vec3 operator/(const vec3 &v, float t) { return (1 / t) * v; }

constexpr float dot(const vec3 &u, const vec3 &v) {
  return u.x * v.x + u.y * v.y + u.z * v.z;
}

constexpr vec3 cross(const vec3 &u, const vec3 &v) {
  return vec3(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
              u.x * v.y - u.y * v.x);
}

constexpr vec3 normalized(const vec3 &v) { return v / v.length(); }

using point3 = vec3;
} // namespace cg

static_assert(std::is_standard_layout_v<cg::vec3>);
static_assert(sizeof(cg::vec3) == 3 * sizeof(float));
