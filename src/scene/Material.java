package scene;

import java.awt.Color;
import math.Vector3D;

public class Material {
  private static final float DEFAULT_SHININESS = 32f;
  private static final float DEFAULT_AMBIENT_FACTOR = 0.1f;
  private static final float MATERIAL_EQ_EPSILON = 1e-8f;
  private final Vector3D ambient;
  private final Vector3D diffuse;
  private final Vector3D specular;
  private final float shininess;

  public Material(Vector3D ambient, Vector3D diffuse, Vector3D specular, float shininess) {
    this.ambient = ambient;
    this.diffuse = diffuse;
    this.specular = specular;
    this.shininess = shininess;
  }

  public static Material fromAlbedo(Color color) {
    Vector3D albedo = fromColor(color);
    return new Material(albedo.mul(DEFAULT_AMBIENT_FACTOR), albedo, new Vector3D(1f, 1f, 1f), DEFAULT_SHININESS);
  }

  public static Vector3D fromColor(Color color) {
    return new Vector3D(
        color.getRed() / 255.99f,
        color.getGreen() / 255.99f,
        color.getBlue() / 255.99f);
  }

  public Vector3D getAmbient() {
    return ambient;
  }

  public Vector3D getDiffuse() {
    return diffuse;
  }

  public Vector3D getSpecular() {
    return specular;
  }

  public float getShininess() {
    return shininess;
  }

  public boolean almostEquals(Material other) {
    return ambient.sub(other.ambient).lengthSquared() <= MATERIAL_EQ_EPSILON
        && diffuse.sub(other.diffuse).lengthSquared() <= MATERIAL_EQ_EPSILON
        && specular.sub(other.specular).lengthSquared() <= MATERIAL_EQ_EPSILON
        && Math.abs(shininess - other.shininess) <= MATERIAL_EQ_EPSILON;
  }
}
