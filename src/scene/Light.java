package scene;

import math.Intersection;
import math.Vector3D;

public interface Light {
  Vector3D shade(Material material, Intersection hit, Vector3D viewDir);
}
