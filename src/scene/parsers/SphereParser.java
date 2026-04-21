
package scene.parsers;

import objects.Object3D;
import objects.Sphere;

import java.awt.Color;
import java.util.Optional;

import math.Vector3D;

/**
 * SphereParser is just the implementation to read a line of a file
 * that get this format
 * Described sphere attributes: sphere x y z radius r g b (r,b,g are 0-255)
 * example: "sphere 0 0 -5 1 255 0 0" for a red sphere at (0,0,-5) with radius 1
 */
public class SphereParser {

  public static Optional<Object3D> parseSphere(String[] tokens) {
    if (tokens.length != 8) {
      System.out.println("Invalid sphere format. Expected: sphere <x> <y> <z> <radius> <r> <g> <b>");
      return Optional.empty();
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

      return Optional.of(new Sphere(new Vector3D(x, y, z), radius, new Color(r, g, b)));
    } catch (Exception e) {
      return Optional.empty();
    }
  }

}
