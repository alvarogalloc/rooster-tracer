package scene.parsers;

import objects.Object3D;
import objects.Triangle;
import math.Vector3D;

import java.awt.Color;
import java.util.Optional;

/**
 * TriangleParser reads a line with the format:
 * triangle x0 y0 z0 x1 y1 z1 x2 y2 z2 r g b
 * Example: "triangle 0 1 0 -1 -1 0 1 -1 0 255 255 255" for a white triangle
 */
public class TriangleParser {

  public static Optional<Object3D> parseTriangle(String[] tokens) {
    if (tokens.length != 13) {
      System.out.println("Invalid triangle format. Expected: triangle <x0> <y0> <z0> <x1> <y1> <z1> <x2> <y2> <z2> <r> <g> <b>");
      return Optional.empty();
    }

    try {
      float x0 = Float.parseFloat(tokens[1]);
      float y0 = Float.parseFloat(tokens[2]);
      float z0 = Float.parseFloat(tokens[3]);
      
      float x1 = Float.parseFloat(tokens[4]);
      float y1 = Float.parseFloat(tokens[5]);
      float z1 = Float.parseFloat(tokens[6]);
      
      float x2 = Float.parseFloat(tokens[7]);
      float y2 = Float.parseFloat(tokens[8]);
      float z2 = Float.parseFloat(tokens[9]);

      // Color components (0-255)
      int r = Integer.parseInt(tokens[10]);
      int g = Integer.parseInt(tokens[11]);
      int b = Integer.parseInt(tokens[12]);

      Vector3D p0 = new Vector3D(x0, y0, z0);
      Vector3D p1 = new Vector3D(x1, y1, z1);
      Vector3D p2 = new Vector3D(x2, y2, z2);

      return Optional.of(new Triangle(p0, p1, p2, new Color(r, g, b)));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing triangle: " + e.getMessage());
      return Optional.empty();
    }
  }
}