package objects;
import java.awt.Color;
import java.util.Optional;
import math.*;

public class Sphere implements Object3D {
  private final Vector3D center;
  private final float radius;
  private final Color color;

  public Sphere(Vector3D center, float radius, Color color) {
    this.center = center;
    this.radius = radius;
    this.color = color;
  }

  public Vector3D getCenter() {
    return center;
  }

  public float getRadius() {
    return radius;
  }

  @Override
  public Color getColor() {
    return color;
  }

  @Override
  public Optional<Intersection> isHit(Ray ray, Interval tRange) {
    Vector3D oc = ray.getPos().add(center.mul(-1));
    // general formula
    float a = ray.getDir().dot(ray.getDir());
    float b = 2.0f * oc.dot(ray.getDir());
    float c = oc.dot(oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    // no solution in R
    if (discriminant < 0) {
      return Optional.empty();
    }

    // has roots
    float t = (-b - (float) Math.sqrt(discriminant)) / (2.0f * a);
    // no roots behind de camera
    if (t <= 0 || !tRange.contains(t)) {
      return Optional.empty();
    }
    Vector3D hitPoint = ray.at(t);
    Vector3D normal = hitPoint.add(center.mul(-1)).normalize();
    return Optional.of(new Intersection(hitPoint, normal, t));
  }

}
