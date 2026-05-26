export module interval;
import std;
export namespace cg
{

struct interval
{
    double min;
    double max;
    constexpr interval()
        : min(+std::numeric_limits<double>::infinity()),
          max(-std::numeric_limits<double>::infinity())
    {
    }

    constexpr interval(double min, double max) : min(min), max(max)
    {
    }

    double size() const
    {
        return max - min;
    }

    bool contains(double x) const
    {
        return min <= x && x <= max;
    }

    bool surrounds(double x) const
    {
        return min < x && x < max;
    }
};

constexpr inline interval empty_interval{};
constexpr inline interval universe{-std::numeric_limits<double>::infinity(),
                                   +std::numeric_limits<double>::infinity()};
} // namespace cg
