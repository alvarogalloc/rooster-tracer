package objects;

import java.awt.Color;
import java.util.Optional;
import math.Intersection;
import math.Ray;

public interface Object3D {
  public Color getColor();

  public Optional<Intersection> isHit(Ray ray);
}
