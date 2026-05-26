package scene.parsers;

import scene.Scene;

public class ViewportParser {
  public static void parseViewport(Scene scene, String[] tokens) {
    if (tokens.length != 3) {
      System.err.println("Invalid viewport format. Expected: viewport <width> <height>");
      return;
    }

    try {
      int width = Integer.parseInt(tokens[1]);
      int height = Integer.parseInt(tokens[2]);
      scene.setViewport(width, height);
    } catch (NumberFormatException e) {
      System.err.println("Error parsing viewport: " + e.getMessage());
    }
  }
}
