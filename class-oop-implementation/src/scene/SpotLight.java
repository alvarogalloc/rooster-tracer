package scene;

import java.awt.Color;
import math.Intersection;
import math.Ray;
import math.Vector3D;

public class SpotLight implements Light {
  private static final float MIN_DISTANCE_SQ = 1e-4f;
  private static final float MIN_NORM_SQ = 1e-8f;
  private final Vector3D pos;
  private final Vector3D dir;
  private final Vector3D color;
  private final float intensity;
  private final float cosAngle;
  private final float radius;
  private final int samples;

  public SpotLight(Vector3D pos, Vector3D dir, Color color, float intensity, float angleDeg) {
    this(pos, dir, color, intensity, angleDeg, 0.0f, 1);
  }

  public SpotLight(Vector3D pos, Vector3D dir, Color color, float intensity, float angleDeg, float radius, int samples) {
    this.pos = pos;
    this.dir = dir.normalize();
    this.color = Material.fromColor(color);
    this.intensity = intensity;
    this.cosAngle = (float) Math.cos(Math.toRadians(angleDeg / 2.0));
    this.radius = radius;
    this.samples = samples;
  }

  public SpotLight(Vector3D pos, Vector3D dir, Vector3D colorVec, float intensity, float angleDeg, float radius, int samples) {
    this.pos = pos;
    this.dir = dir.normalize();
    this.color = colorVec;
    this.intensity = intensity;
    this.cosAngle = (float) Math.cos(Math.toRadians(angleDeg / 2.0));
    this.radius = radius;
    this.samples = samples;
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

    final float cosTheta = l.mul(-1f).dot(dir);
    float spotAttenuation = 0.0f;
    if (cosTheta >= cosAngle) {
      spotAttenuation = 1.0f;
    }

    if (spotAttenuation <= 0.0f) {
      return new Vector3D(0f, 0f, 0f);
    }

    final float lambert = Math.max(0f, n.dot(l));
    final Vector3D reflection = safeNormalize(l.mul(-1f).sub(n.mul(2f * l.mul(-1f).dot(n))));
    final float specBase = Math.max(0f, v.dot(reflection));
    final float specular = (float) Math.pow(specBase, Math.max(1f, material.getShininess()));
    final Vector3D radiance = color.mul((intensity / distSq) * spotAttenuation);
    return radiance.vec_mul(material.getDiffuse().mul(lambert).add(material.getSpecular().mul(specular)));
  }

  @Override
  public ShadowRay shadowRay(Intersection hit) {
    Vector3D toLight = pos.sub(hit.getPoint());
    float distance = (float) Math.sqrt(toLight.lengthSquared());
    Vector3D origin = hit.getPoint().add(hit.getNormal().normalize().mul(Light.SHADOW_BIAS));
    return new ShadowRay(new Ray(origin, toLight), distance);
  }

  public Vector3D getPos() {
    return pos;
  }

  public Vector3D getDir() {
    return dir;
  }

  public Vector3D getColor() {
    return color;
  }

  public float getIntensity() {
    return intensity;
  }

  public float getCosAngle() {
    return cosAngle;
  }

  public float getRadius() {
    return radius;
  }

  public int getSamples() {
    return samples;
  }
}
