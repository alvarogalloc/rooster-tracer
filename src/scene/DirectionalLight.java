package scene;

import java.awt.Color;
import math.Intersection;
import math.Ray;
import math.Vector3D;

public class DirectionalLight implements Light {
  private static final float MIN_NORM_SQ = 1e-8f;
  private final Vector3D dir;
  private final Vector3D color;
  private final float intensity;

  public DirectionalLight(Vector3D dir, Color color, float intensity) {
    this.dir = dir.normalize();
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
    final Vector3D n = safeNormalize(hit.getNormal());
    final Vector3D l = safeNormalize(dir.mul(-1f));
    final Vector3D v = safeNormalize(viewDir);
    final float lambert = Math.max(0f, n.dot(l));
    final Vector3D reflection = safeNormalize(l.mul(-1f).sub(n.mul(2f * l.mul(-1f).dot(n))));
    final float specBase = Math.max(0f, v.dot(reflection));
    final float specular = (float) Math.pow(specBase, Math.max(1f, material.getShininess()));
    final Vector3D radiance = color.mul(intensity);
    return radiance.vec_mul(material.getDiffuse().mul(lambert).add(material.getSpecular().mul(specular)));
  }

  @Override
  public ShadowRay shadowRay(Intersection hit) {
    Vector3D origin = hit.getPoint().add(hit.getNormal().normalize().mul(1e-3f));
    Vector3D toLight = dir.mul(-1f);
    return new ShadowRay(new Ray(origin, toLight), Float.POSITIVE_INFINITY);
  }
}
