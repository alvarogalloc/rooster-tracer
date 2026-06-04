package scene.parsers;

import java.awt.Color;
import math.Vector3D;
import scene.Scene;
import scene.SpotLight;

public class SpotLightParser {
  public static void parseSpotLight(Scene scene, String[] tokens) {
    if (tokens.length < 11) {
      System.err.println(
          "Invalid spot_light format. Expected: spot_light <px> <py> <pz> <dx> <dy> <dz> <r> <g> <b> <intensity> <angle> [<radius> <samples>]");
      return;
    }

    try {
      float px = Float.parseFloat(tokens[1]);
      float py = Float.parseFloat(tokens[2]);
      float pz = Float.parseFloat(tokens[3]);
      float dx = Float.parseFloat(tokens[4]);
      float dy = Float.parseFloat(tokens[5]);
      float dz = Float.parseFloat(tokens[6]);
      int r = Integer.parseInt(tokens[7]);
      int g = Integer.parseInt(tokens[8]);
      int b = Integer.parseInt(tokens[9]);
      float intensity = Float.parseFloat(tokens[10]);
      float angle = Float.parseFloat(tokens[11]);

      float radius = 0.0f;
      if (tokens.length > 12) {
        radius = Float.parseFloat(tokens[12]);
      }

      int samples = 1;
      if (tokens.length > 13) {
        samples = Integer.parseInt(tokens[13]);
      }

      scene.addLight(new SpotLight(
          new Vector3D(px, py, pz),
          new Vector3D(dx, dy, dz),
          new Color(r, g, b),
          intensity,
          angle,
          radius,
          samples
      ));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing spot light: " + e.getMessage());
    }
  }
}
