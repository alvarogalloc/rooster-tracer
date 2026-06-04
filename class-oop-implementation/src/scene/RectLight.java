package scene;

import java.awt.Color;
import math.Intersection;
import math.Ray;
import math.Vector3D;

public class RectLight implements Light {
  private final Vector3D pos;
  private final Vector3D u;
  private final Vector3D v;
  private final Vector3D color;
  private final float intensity;
  private final int samples;
  private final Vector3D normal;

  public RectLight(Vector3D pos, Vector3D u, Vector3D v, Color color, float intensity, int samples) {
    this.pos = pos;
    this.u = u;
    this.v = v;
    this.color = Material.fromColor(color);
    this.intensity = intensity;
    this.samples = samples;
    this.normal = u.cross(v).normalize();
  }

  public RectLight(Vector3D pos, Vector3D u, Vector3D v, Vector3D colorVec, float intensity, int samples) {
    this.pos = pos;
    this.u = u;
    this.v = v;
    this.color = colorVec;
    this.intensity = intensity;
    this.samples = samples;
    this.normal = u.cross(v).normalize();
  }

  @Override
  public Vector3D shade(Material material, Intersection hit, Vector3D viewDir) {
    // Should be handled by Raytracer's custom Monte Carlo loop
    return new Vector3D(0f, 0f, 0f);
  }

  @Override
  public ShadowRay shadowRay(Intersection hit) {
    // Return dummy ray of zero length to avoid shadow testing on generic path
    return new ShadowRay(new Ray(hit.getPoint(), new Vector3D(0f, 0f, 0f)), 0.0f);
  }

  public Vector3D getPos() {
    return pos;
  }

  public Vector3D getU() {
    return u;
  }

  public Vector3D getV() {
    return v;
  }

  public Vector3D getColor() {
    return color;
  }

  public float getIntensity() {
    return intensity;
  }

  public int getSamples() {
    return samples;
  }

  public Vector3D getNormal() {
    return normal;
  }
}
