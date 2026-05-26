package scene.parsers;

import java.awt.Color;
import math.Vector3D;
import scene.PointLight;
import scene.Scene;

public class PointLightParser {
  public static void parsePointLight(Scene scene, String[] tokens) {
    if (tokens.length != 8) {
      System.err.println(
          "Invalid point_light format. Expected: point_light <px> <py> <pz> <r> <g> <b> <intensity>");
      return;
    }

    try {
      float px = Float.parseFloat(tokens[1]);
      float py = Float.parseFloat(tokens[2]);
      float pz = Float.parseFloat(tokens[3]);
      int r = Integer.parseInt(tokens[4]);
      int g = Integer.parseInt(tokens[5]);
      int b = Integer.parseInt(tokens[6]);
      float intensity = Float.parseFloat(tokens[7]);
      scene.addLight(new PointLight(new Vector3D(px, py, pz), new Color(r, g, b), intensity));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing point light: " + e.getMessage());
    }
  }
}
