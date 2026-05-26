package scene.parsers;

import math.Vector3D;
import objects.Plane;
import scene.Scene;

public class PlaneParser {
  public static void parsePlane(Scene scene, String[] tokens) {
    if (tokens.length != 8) {
      System.err.println("Invalid plane format. Expected: plane <px> <py> <pz> <nx> <ny> <nz> <material_id>");
      return;
    }

    try {
      float px = Float.parseFloat(tokens[1]);
      float py = Float.parseFloat(tokens[2]);
      float pz = Float.parseFloat(tokens[3]);
      float nx = Float.parseFloat(tokens[4]);
      float ny = Float.parseFloat(tokens[5]);
      float nz = Float.parseFloat(tokens[6]);
      int materialId = Integer.parseInt(tokens[7]);
      if (materialId < 0 || materialId >= scene.getMaterials().size()) {
        System.err.println(
            "Invalid plane format. Expected material_id in range [0, " + scene.getMaterials().size() + ")");
        return;
      }
      scene.add(new Plane(new Vector3D(px, py, pz), new Vector3D(nx, ny, nz), materialId));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing plane: " + e.getMessage());
    }
  }
}
