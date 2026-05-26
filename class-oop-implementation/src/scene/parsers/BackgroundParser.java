package scene.parsers;

import java.awt.Color;
import scene.Scene;

public class BackgroundParser {
  public static void parseBackground(Scene scene, String[] tokens) {
    if (tokens.length != 4) {
      System.err.println("Invalid background format. Expected: background <r> <g> <b>");
      return;
    }

    try {
      int r = Integer.parseInt(tokens[1]);
      int g = Integer.parseInt(tokens[2]);
      int b = Integer.parseInt(tokens[3]);
      scene.setBackground(new Color(r, g, b));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing background: " + e.getMessage());
    }
  }
}
