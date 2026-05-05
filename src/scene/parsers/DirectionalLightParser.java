package scene.parsers;

import java.awt.Color;
import math.Vector3D;
import scene.DirectionalLight;
import scene.Scene;

public class DirectionalLightParser {
  public static void parseDirectionalLight(Scene scene, String[] tokens) {
    if (tokens.length != 8) {
      System.err.println(
          "Invalid dir_light format. Expected: dir_light <dx> <dy> <dz> <r> <g> <b> <intensity>");
      return;
    }

    try {
      float dx = Float.parseFloat(tokens[1]);
      float dy = Float.parseFloat(tokens[2]);
      float dz = Float.parseFloat(tokens[3]);
      int r = Integer.parseInt(tokens[4]);
      int g = Integer.parseInt(tokens[5]);
      int b = Integer.parseInt(tokens[6]);
      float intensity = Float.parseFloat(tokens[7]);
      scene.addLight(new DirectionalLight(new Vector3D(dx, dy, dz), new Color(r, g, b), intensity));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing directional light: " + e.getMessage());
    }
  }
}
