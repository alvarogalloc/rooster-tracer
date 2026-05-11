package objects;

import java.util.Optional;
import math.Intersection;
import math.Interval;
import math.Ray;
import math.Vector3D;

public class Plane implements Object3D {
  private static final Interval ALMOST_ZERO = new Interval(-1e-6f, 1e-6f);
  private final Vector3D normal;
  private final Vector3D point;
  private final int materialId;

  public Plane(Vector3D point, Vector3D normal, int materialId) {
    this.point = point;
    this.normal = normal.normalize();
    this.materialId = materialId;
  }

  @Override
  public Optional<Intersection> isHit(Ray ray, Interval tRange) {
    final float denominator = normal.dot(ray.getDir());
    if (ALMOST_ZERO.contains(denominator)) {
      return Optional.empty();
    }

    final float t = normal.dot(point.sub(ray.getPos())) / denominator;
    if (t < 0f || !tRange.contains(t)) {
      return Optional.empty();
    }

    final Vector3D hitNormal = denominator < 0f ? normal : normal.mul(-1f);
    return Optional.of(new Intersection(ray.at(t), hitNormal, t, materialId));
  }
}
