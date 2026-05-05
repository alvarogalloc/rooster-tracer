package scene;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.awt.Color;

import objects.Object3D;

public class Scene {
  private List<Object3D> objects;
  private List<DirectionalLight> lights;
  private List<Color> materials;

  public List<Object3D> getObjects() {
    return objects;
  }
  public List<DirectionalLight> getLights() {
    return lights;
  }
  public List<Color> getMaterials() {
    return materials;
  }

  public Scene() {
    this.objects = new ArrayList<>();
    this.lights = new ArrayList<>();
    this.materials = new ArrayList<>();
  }

  public void add(Object3D obj) {
    this.objects.add(obj);
  }
  public void addLight(DirectionalLight light) {
    this.lights.add(light);
  }
  public void addMaterial(Color material) {
    this.materials.add(material);
  }
  public void addCollection(Collection<Object3D> objs) {
    this.objects.addAll(objs);
  }
}
