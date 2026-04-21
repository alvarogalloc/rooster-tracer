import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

public class Scene {
  private List<Object3D> objects;

  public List<Object3D> getObjects() {
    return objects;
  }

  public Scene() {
    this.objects = new ArrayList<>();
  }

  public void add(Object3D obj) {
    this.objects.add(obj);
  }
  public void addCollection(Collection<Object3D> objs) {
    this.objects.addAll(objs);
  }
}
