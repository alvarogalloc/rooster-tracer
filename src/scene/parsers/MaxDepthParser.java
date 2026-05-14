package scene.parsers;

import scene.Scene;

public class MaxDepthParser {
  public static void parseMaxDepth(Scene scene, String[] tokens) {
    if (tokens.length != 2) {
      System.err.println("Invalid max_depth format. Expected: max_depth <integer>");
      return;
    }

    try {
      int maxDepth = Integer.parseInt(tokens[1]);
      scene.setMaxDepth(maxDepth);
    } catch (NumberFormatException e) {
      System.err.println("Error parsing max_depth: " + e.getMessage());
    }
  }
}
