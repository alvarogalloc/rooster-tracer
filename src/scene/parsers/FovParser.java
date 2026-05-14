package scene.parsers;

import scene.Scene;

public class FovParser {
  public static void parseFov(Scene scene, String[] tokens) {
    if (tokens.length != 2) {
      System.err.println("Invalid fov format. Expected: fov <degrees>");
      return;
    }

    try {
      float degrees = Float.parseFloat(tokens[1]);
      scene.setFov((float) Math.toRadians(degrees));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing fov: " + e.getMessage());
    }
  }
}
