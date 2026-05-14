package scene.parsers;

import math.Vector3D;
import scene.Scene;

public class CameraParser {
  public static void parseCamera(Scene scene, String[] tokens) {
    if (tokens.length != 10 && tokens.length != 12) {
      System.err.println(
          "Invalid camera format. Expected: camera <px> <py> <pz> <lookx> <looky> <lookz> <upx> <upy> <upz> [<near> <far>]");
      return;
    }

    try {
      Vector3D pos = new Vector3D(
          Float.parseFloat(tokens[1]),
          Float.parseFloat(tokens[2]),
          Float.parseFloat(tokens[3]));
      Vector3D lookAt = new Vector3D(
          Float.parseFloat(tokens[4]),
          Float.parseFloat(tokens[5]),
          Float.parseFloat(tokens[6]));
      Vector3D up = new Vector3D(
          Float.parseFloat(tokens[7]),
          Float.parseFloat(tokens[8]),
          Float.parseFloat(tokens[9]));

      float nearPlane = scene.getNearPlane();
      float farPlane = scene.getFarPlane();
      if (tokens.length == 12) {
        nearPlane = Float.parseFloat(tokens[10]);
        farPlane = Float.parseFloat(tokens[11]);
      }

      scene.setCamera(pos, lookAt, up, nearPlane, farPlane);
    } catch (NumberFormatException e) {
      System.err.println("Error parsing camera: " + e.getMessage());
    }
  }
}
