package math;

public final class Interval {
  final private float min;
  final private float max;
  Interval(float mn, float mx) {
    this.min = mn;
    this.max = mx;
  }

  public float size() {
    return max - min;
  }

  public boolean contains(float x) {
    return min <= x && x <= max;
  }

  public boolean surrounds(float x) {
    return min < x && x < max;
  }
  public static final  Interval empty = new Interval(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY);
  public static final  Interval universe = new Interval(Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY);
}
