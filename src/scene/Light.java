package scene;

import math.Intersection;
import math.Ray;
import math.Vector3D;

public interface Light {
  record ShadowRay(Ray ray, float maxDistance) {
  }

  Vector3D shade(Material material, Intersection hit, Vector3D viewDir);

  ShadowRay shadowRay(Intersection hit);
}
