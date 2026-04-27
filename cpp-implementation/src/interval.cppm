export module interval;
import std;
export namespace cg {

struct interval {
  float min;
  float max;
  constexpr interval()
      : min(+std::numeric_limits<float>::infinity()),
        max(-std::numeric_limits<float>::infinity()) {}

  constexpr interval(float min, float max) : min(min), max(max) {}

  float size() const { return max - min; }

  bool contains(float x) const { return min <= x && x <= max; }

  bool surrounds(float x) const { return min < x && x < max; }
};

constexpr inline interval empty{};
constexpr inline interval universe{-std::numeric_limits<float>::infinity(),
                                   +std::numeric_limits<float>::infinity()};
} // namespace cg
