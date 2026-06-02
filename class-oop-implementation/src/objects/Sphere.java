package objects;
import java.util.Optional;
import math.*;

public class Sphere implements Object3D {
  private final Vector3D center;
  private final float radius;
  private final int materialId;

  public Sphere(Vector3D center, float radius, int materialId) {
    this.center = center;
    this.radius = radius;
    this.materialId = materialId;
  }

  public Vector3D getCenter() {
    return center;
  }

  public float getRadius() {
    return radius;
  }

  @Override
  public Optional<Intersection> isHit(Ray ray, Interval tRange) {
    Vector3D dirToSphere = center.sub(ray.getPos());
    float a = ray.getDir().dot(ray.getDir());
    float h = ray.getDir().dot(dirToSphere);
    float c = dirToSphere.dot(dirToSphere) - radius * radius;

    float discriminant = h * h - a * c;
    if (discriminant < 0) {
      return Optional.empty();
    }

    float sqrtDiscriminant = (float) Math.sqrt(discriminant);
    float t = (h - sqrtDiscriminant) / a;
    if (t < 0 || !tRange.contains(t)) {
      t = (h + sqrtDiscriminant) / a;
      if (t < 0 || !tRange.contains(t)) {
        return Optional.empty();
      }
    }

    Vector3D hitPoint = ray.at(t);
    Vector3D normal = hitPoint.sub(center).mul(1f / radius).normalize();
    if (normal.dot(ray.getDir()) > 0f) {
      normal = normal.mul(-1f);
    }
    return Optional.of(new Intersection(hitPoint, normal, t, materialId));
  }

}
