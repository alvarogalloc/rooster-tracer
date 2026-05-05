package scene.parsers;

import java.awt.Color;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import math.Vector3D;
import objects.Triangle;
import scene.Scene;

public class ObjParser {
  public static void parseObj(Scene scene, String[] tokens) throws IOException {
    if (tokens.length < 2) {
      System.err.println("Invalid obj format. Expected: obj <path/to/model.obj>");
      return;
    }

    String filename = tokens[1];
    Vector3D origin = new Vector3D(0, 0, 0);
    if (tokens.length >= 5) {
      try {
        origin = new Vector3D(
            Float.parseFloat(tokens[2]),
            Float.parseFloat(tokens[3]),
            Float.parseFloat(tokens[4]));
      } catch (NumberFormatException e) {
        throw new IOException("Error parsing OBJ origin: " + e.getMessage(), e);
      }
    }

    Integer materialId = null;
    if (tokens.length >= 6) {
      try {
        materialId = Integer.parseInt(tokens[5]);
      } catch (NumberFormatException e) {
        throw new IOException("Error parsing OBJ material id: " + e.getMessage(), e);
      }
    }
    Color faceColor = resolveFaceColor(scene, materialId);

    System.out.println("loading obj: " + filename);

    List<Vector3D> vertices = new ArrayList<>();
    int objectsBefore = scene.getObjects().size();

    try (BufferedReader reader = new BufferedReader(new FileReader(filename))) {
      String line;
      while ((line = reader.readLine()) != null) {
        line = line.trim();
        if (line.isEmpty() || line.startsWith("#")) {
          continue;
        }

        String[] objTokens = line.split("\\s+");
        switch (objTokens[0]) {
          case "v":
            parseVertex(objTokens, vertices, origin);
            break;
          case "f":
            parseFace(objTokens, vertices, scene, faceColor);
            break;
          default:
            break;
        }
      }
    }

    int generatedFaces = scene.getObjects().size() - objectsBefore;
    System.out.println(
        "done loading model (vertex count: " + vertices.size() + ", face count: " + generatedFaces + ")");
  }

  private static Color resolveFaceColor(Scene scene, Integer materialId) throws IOException {
    if (materialId != null) {
      if (scene.getMaterials().isEmpty()) {
        throw new IOException("obj parser: material index provided but no materials are defined");
      }
      if (materialId < 0 || materialId >= scene.getMaterials().size()) {
        throw new IOException("obj parser: material index " + materialId + " out of range [0, "
            + scene.getMaterials().size() + ")");
      }
      return scene.getMaterials().get(materialId);
    }
    if (!scene.getMaterials().isEmpty()) {
      return scene.getMaterials().get(0);
    }
    return new Color(255, 255, 255);
  }

  private static void parseVertex(String[] tokens, List<Vector3D> vertices, Vector3D origin) {
    if (tokens.length < 4) {
      return;
    }

    try {
      float x = Float.parseFloat(tokens[1]);
      float y = Float.parseFloat(tokens[2]);
      float z = Float.parseFloat(tokens[3]);
      vertices.add(new Vector3D(x, y, z).add(origin.mul(-1)));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing vertex: " + e.getMessage());
    }
  }

  private static void parseFace(String[] tokens, List<Vector3D> vertices, Scene scene, Color faceColor) {
    if (tokens.length < 4) {
      return;
    }

    int[] indices = new int[3];

    for (int i = 0; i < 3; i++) {
      String faceToken = tokens[i + 1];
      int separator = faceToken.indexOf('/');
      String indexToken = separator >= 0 ? faceToken.substring(0, separator) : faceToken;

      try {
        indices[i] = Integer.parseInt(indexToken) - 1;
      } catch (NumberFormatException e) {
        return;
      }

      if (indices[i] < 0 || indices[i] >= vertices.size()) {
        return;
      }
    }

    scene.add(new Triangle(
        vertices.get(indices[0]),
        vertices.get(indices[1]),
        vertices.get(indices[2]),
        faceColor));
  }
}
