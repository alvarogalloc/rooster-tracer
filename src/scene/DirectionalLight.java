package scene;

import java.awt.Color;
import math.Vector3D;

public class DirectionalLight {
  private final Vector3D dir;
  private final Color color;
  private final float intensity;

  public DirectionalLight(Vector3D dir, Color color, float intensity) {
    this.dir = dir.normalize();
    this.color = color;
    this.intensity = intensity;
  }

  public Vector3D getDir() {
    return dir;
  }

  public Color getColor() {
    return color;
  }

  public float getIntensity() {
    return intensity;
  }
}
