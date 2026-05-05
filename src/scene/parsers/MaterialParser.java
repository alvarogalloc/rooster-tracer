package scene.parsers;

import java.awt.Color;
import scene.Scene;

public class MaterialParser {
  public static void parseMaterial(Scene scene, String[] tokens) {
    if (tokens.length != 4) {
      System.err.println("Invalid mat format. Expected: mat <r> <g> <b>");
      return;
    }

    try {
      int r = Integer.parseInt(tokens[1]);
      int g = Integer.parseInt(tokens[2]);
      int b = Integer.parseInt(tokens[3]);
      scene.addMaterial(new Color(r, g, b));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing material: " + e.getMessage());
    }
  }
}
