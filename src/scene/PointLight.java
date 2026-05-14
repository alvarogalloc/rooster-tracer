package scene;

import java.awt.Color;
import math.Intersection;
import math.Ray;
import math.Vector3D;

public class PointLight implements Light {
  private static final float MIN_DISTANCE_SQ = 1e-4f;
  private static final float MIN_NORM_SQ = 1e-8f;
  private final Vector3D pos;
  private final Vector3D color;
  private final float intensity;

  public PointLight(Vector3D pos, Color color, float intensity) {
    this.pos = pos;
    this.color = Material.fromColor(color);
    this.intensity = intensity;
  }

  private static Vector3D safeNormalize(Vector3D v) {
    if (v.lengthSquared() <= MIN_NORM_SQ) {
      return new Vector3D(0f, 0f, 0f);
    }
    return v.normalize();
  }

  @Override
  public Vector3D shade(Material material, Intersection hit, Vector3D viewDir) {
    final Vector3D toLight = pos.sub(hit.getPoint());
    final float distSq = Math.max(MIN_DISTANCE_SQ, toLight.lengthSquared());
    final Vector3D n = safeNormalize(hit.getNormal());
    final Vector3D l = safeNormalize(toLight);
    final Vector3D v = safeNormalize(viewDir);
    final float lambert = Math.max(0f, n.dot(l));
    final Vector3D reflection = safeNormalize(l.mul(-1f).sub(n.mul(2f * l.mul(-1f).dot(n))));
    final float specBase = Math.max(0f, v.dot(reflection));
    final float specular = (float) Math.pow(specBase, Math.max(1f, material.getShininess()));
    final Vector3D radiance = color.mul(intensity / distSq);
    return radiance.vec_mul(material.getDiffuse().mul(lambert).add(material.getSpecular().mul(specular)));
  }

  @Override
  public ShadowRay shadowRay(Intersection hit) {
    Vector3D toLight = pos.sub(hit.getPoint());
    float distance = (float) Math.sqrt(toLight.lengthSquared());
    Vector3D origin = hit.getPoint().add(hit.getNormal().normalize().mul(1e-3f));
    return new ShadowRay(new Ray(origin, toLight), distance);
  }
}
