package objects;

import java.util.Optional;
import math.Intersection;
import math.Interval;
import math.Ray;

public interface Object3D {
  public Optional<Intersection> isHit(Ray ray, Interval tRange);
}
