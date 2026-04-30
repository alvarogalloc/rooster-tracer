
package scene.parsers;

import objects.Sphere;
import scene.Scene;

import java.awt.Color;

import math.Vector3D;

/**
 * SphereParser is just the implementation to read a line of a file
 * that get this format
 * Described sphere attributes: sphere x y z radius r g b (r,b,g are 0-255)
 * example: "sphere 0 0 -5 1 255 0 0" for a red sphere at (0,0,-5) with radius 1
 */
public class SphereParser {

  public static void parseSphere(Scene scene, String[] tokens) {
    if (tokens.length != 8) {
      System.err.println("Invalid sphere format. Expected: sphere <x> <y> <z> <radius> <r> <g> <b>");
      return;
    }

    try {

      float x = Float.parseFloat(tokens[1]);
      float y = Float.parseFloat(tokens[2]);
      float z = Float.parseFloat(tokens[3]);
      float radius = Float.parseFloat(tokens[4]);

      // Expected color components: integers from 0 to 255
      int r = Integer.parseInt(tokens[5]);
      int g = Integer.parseInt(tokens[6]);
      int b = Integer.parseInt(tokens[7]);

      scene.add(new Sphere(new Vector3D(x, y, z), radius, new Color(r, g, b)));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing sphere: " + e.getMessage());
    }
  }

}
