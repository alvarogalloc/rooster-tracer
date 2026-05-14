package scene;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.awt.Color;
import math.Vector3D;

import objects.Object3D;

public class Scene {
  private static final int DEFAULT_IMAGE_WIDTH = 1280;
  private static final int DEFAULT_IMAGE_HEIGHT = 720;
  private static final float DEFAULT_FOV = 1.0472f;
  private static final Vector3D DEFAULT_CAMERA_POS = new Vector3D(0f, 2f, 5f);
  private static final Vector3D DEFAULT_CAMERA_UP = new Vector3D(0f, 1f, 0f);
  private static final Vector3D DEFAULT_CAMERA_LOOK_AT = new Vector3D(0f, -1f, -4f);
  private static final float DEFAULT_NEAR_PLANE = 0.001f;
  private static final float DEFAULT_FAR_PLANE = 20f;
  private static final Color DEFAULT_BACKGROUND = new Color(10, 32, 90);
  private static final int DEFAULT_MAX_DEPTH = 5;

  private List<Object3D> objects;
  private List<Light> lights;
  private List<Material> materials;
  private int imageWidth;
  private int imageHeight;
  private float fov;
  private Vector3D cameraPos;
  private Vector3D cameraUp;
  private Vector3D cameraLookAt;
  private float nearPlane;
  private float farPlane;
  private Color background;
  private int maxDepth;
  private String sourceDir;

  public List<Object3D> getObjects() {
    return objects;
  }
  public List<Light> getLights() {
    return lights;
  }
  public List<Material> getMaterials() {
    return materials;
  }

  public Scene() {
    this.objects = new ArrayList<>();
    this.lights = new ArrayList<>();
    this.materials = new ArrayList<>();
    this.imageWidth = DEFAULT_IMAGE_WIDTH;
    this.imageHeight = DEFAULT_IMAGE_HEIGHT;
    this.fov = DEFAULT_FOV;
    this.cameraPos = DEFAULT_CAMERA_POS;
    this.cameraUp = DEFAULT_CAMERA_UP;
    this.cameraLookAt = DEFAULT_CAMERA_LOOK_AT;
    this.nearPlane = DEFAULT_NEAR_PLANE;
    this.farPlane = DEFAULT_FAR_PLANE;
    this.background = DEFAULT_BACKGROUND;
    this.maxDepth = DEFAULT_MAX_DEPTH;
    this.sourceDir = ".";
  }

  public void add(Object3D obj) {
    this.objects.add(obj);
  }
  public void addLight(Light light) {
    this.lights.add(light);
  }
  public void addMaterial(Material material) {
    this.materials.add(material);
  }

  public int addInlineMaterial(Color color) {
    final Material material = Material.fromAlbedo(color);
    for (int i = 0; i < materials.size(); i++) {
      if (materials.get(i).almostEquals(material)) {
        return i;
      }
    }
    materials.add(material);
    return materials.size() - 1;
  }

  public void ensureDefaultMaterial() {
    if (materials.isEmpty()) {
      materials.add(Material.fromAlbedo(new Color(255, 255, 255)));
    }
  }
  public void addCollection(Collection<Object3D> objs) {
    this.objects.addAll(objs);
  }

  public int getImageWidth() {
    return imageWidth;
  }

  public int getImageHeight() {
    return imageHeight;
  }

  public float getFov() {
    return fov;
  }

  public Vector3D getCameraPos() {
    return cameraPos;
  }

  public Vector3D getCameraUp() {
    return cameraUp;
  }

  public Vector3D getCameraLookAt() {
    return cameraLookAt;
  }

  public float getNearPlane() {
    return nearPlane;
  }

  public float getFarPlane() {
    return farPlane;
  }

  public Color getBackground() {
    return background;
  }

  public int getMaxDepth() {
    return maxDepth;
  }

  public String getSourceDir() {
    return sourceDir;
  }

  public void setViewport(int width, int height) {
    this.imageWidth = width;
    this.imageHeight = height;
  }

  public void setFov(float fov) {
    this.fov = fov;
  }

  public void setCamera(Vector3D pos, Vector3D lookAt, Vector3D up, float nearPlane, float farPlane) {
    this.cameraPos = pos;
    this.cameraLookAt = lookAt;
    this.cameraUp = up;
    this.nearPlane = nearPlane;
    this.farPlane = farPlane;
  }

  public void setBackground(Color background) {
    this.background = background;
  }

  public void setMaxDepth(int maxDepth) {
    this.maxDepth = Math.max(1, maxDepth);
  }

  public void setSourceDir(String sourceDir) {
    this.sourceDir = sourceDir;
  }

  public String validateForRender() {
    if (imageWidth <= 0 || imageHeight <= 0) {
      return "viewport width and height must be > 0";
    }
    if (fov <= 0f || fov >= Math.PI) {
      return "fov must be in (0, PI) radians";
    }
    if (nearPlane <= 0f) {
      return "near plane must be > 0";
    }
    if (farPlane <= nearPlane) {
      return "far plane must be greater than near plane";
    }
    if (maxDepth < 1) {
      return "max_depth must be >= 1";
    }
    if (objects.isEmpty()) {
      return "scene must contain at least one object";
    }
    if (materials.isEmpty()) {
      return "scene must contain at least one material";
    }
    if (lights.isEmpty()) {
      return "scene must contain at least one light";
    }
    return null;
  }
}
