package scene;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.awt.Color;

import objects.Object3D;

public class Scene {
  private List<Object3D> objects;
  private List<Light> lights;
  private List<Material> materials;

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
}
