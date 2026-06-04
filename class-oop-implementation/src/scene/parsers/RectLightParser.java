package scene.parsers;

import java.awt.Color;
import math.Vector3D;
import scene.Scene;
import scene.RectLight;

public class RectLightParser {
  public static void parseRectLight(Scene scene, String[] tokens) {
    if (tokens.length < 13) {
      System.err.println(
          "Invalid rect_light format. Expected: rect_light <px> <py> <pz> <ux> <uy> <uz> <vx> <vy> <vz> <r> <g> <b> [<intensity> <samples>]");
      return;
    }

    try {
      float px = Float.parseFloat(tokens[1]);
      float py = Float.parseFloat(tokens[2]);
      float pz = Float.parseFloat(tokens[3]);

      float ux = Float.parseFloat(tokens[4]);
      float uy = Float.parseFloat(tokens[5]);
      float uz = Float.parseFloat(tokens[6]);

      float vx = Float.parseFloat(tokens[7]);
      float vy = Float.parseFloat(tokens[8]);
      float vz = Float.parseFloat(tokens[9]);

      int r = Integer.parseInt(tokens[10]);
      int g = Integer.parseInt(tokens[11]);
      int b = Integer.parseInt(tokens[12]);

      float intensity = 1.0f;
      if (tokens.length > 13) {
        intensity = Float.parseFloat(tokens[13]);
      }

      int samples = 16;
      if (tokens.length > 14) {
        samples = Integer.parseInt(tokens[14]);
      }

      scene.addLight(new RectLight(
          new Vector3D(px, py, pz),
          new Vector3D(ux, uy, uz),
          new Vector3D(vx, vy, vz),
          new Color(r, g, b),
          intensity,
          samples
      ));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing rect light: " + e.getMessage());
    }
  }
}
