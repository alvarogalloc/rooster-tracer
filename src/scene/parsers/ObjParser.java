package scene.parsers;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import math.Vector3D;
import objects.Triangle;
import scene.Scene;

public class ObjParser {
  private static final int MISSING_INDEX = -1;
  private static final float NORMAL_EPSILON = 1e-12f;

  private record FaceVertexRef(int vertexIdx, int normalIdx) {
  }

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

    Integer materialIdArg = null;
    if (tokens.length >= 6) {
      try {
        materialIdArg = Integer.parseInt(tokens[5]);
      } catch (NumberFormatException e) {
        throw new IOException("Error parsing OBJ material id: " + e.getMessage(), e);
      }
    }
    int materialId = resolveMaterialId(scene, materialIdArg);

    System.out.println("loading obj: " + filename);

    List<Vector3D> vertices = new ArrayList<>();
    List<Vector3D> objNormals = new ArrayList<>();
    List<List<FaceVertexRef>> faces = new ArrayList<>();

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
          case "vn":
            parseNormal(objTokens, objNormals);
            break;
          case "f":
            parseFace(objTokens, vertices.size(), objNormals.size(), faces);
            break;
          default:
            break;
        }
      }
    }

    List<Vector3D> generatedNormals = buildGeneratedNormals(vertices, faces);
    int objectsBefore = scene.getObjects().size();
    triangulateFaces(scene, vertices, objNormals, faces, generatedNormals, materialId);
    int generatedFaces = scene.getObjects().size() - objectsBefore;

    System.out.println(
        "done loading model (vertex count: " + vertices.size() + ", face count: " + generatedFaces + ")");
  }

  private static int resolveMaterialId(Scene scene, Integer materialIdArg) throws IOException {
    if (materialIdArg != null) {
      if (scene.getMaterials().isEmpty()) {
        throw new IOException("obj parser: material index provided but no materials are defined");
      }
      if (materialIdArg < 0 || materialIdArg >= scene.getMaterials().size()) {
        throw new IOException("obj parser: material index " + materialIdArg + " out of range [0, "
            + scene.getMaterials().size() + ")");
      }
      return materialIdArg;
    }
    if (scene.getMaterials().isEmpty()) {
      throw new IOException("obj parser: material out of bounds");
    }
    return 0;
  }

  private static void parseVertex(String[] tokens, List<Vector3D> vertices, Vector3D origin) {
    if (tokens.length < 4) {
      return;
    }

    try {
      float x = Float.parseFloat(tokens[1]);
      float y = Float.parseFloat(tokens[2]);
      float z = Float.parseFloat(tokens[3]);
      vertices.add(new Vector3D(x, y, z).sub(origin));
    } catch (NumberFormatException e) {
      System.err.println("Error parsing vertex: " + e.getMessage());
    }
  }

  private static void parseNormal(String[] tokens, List<Vector3D> normals) {
    if (tokens.length < 4) {
      return;
    }
    try {
      float x = Float.parseFloat(tokens[1]);
      float y = Float.parseFloat(tokens[2]);
      float z = Float.parseFloat(tokens[3]);
      normals.add(new Vector3D(x, y, z).normalize());
    } catch (NumberFormatException e) {
      System.err.println("Error parsing normal: " + e.getMessage());
    }
  }

  private static int toZeroBasedIndex(int rawIndex, int count) {
    if (rawIndex > 0) {
      return rawIndex - 1;
    }
    if (rawIndex < 0) {
      return count + rawIndex;
    }
    return MISSING_INDEX;
  }

  private static FaceVertexRef parseFaceToken(String token, int vertexCount, int normalCount) throws IOException {
    int firstSlash = token.indexOf('/');
    int secondSlash = firstSlash == -1 ? -1 : token.indexOf('/', firstSlash + 1);

    String vertexPart = firstSlash == -1 ? token : token.substring(0, firstSlash);
    if (vertexPart.isEmpty()) {
      throw new IOException("obj parser: malformed face vertex index");
    }

    int rawVertex = Integer.parseInt(vertexPart);
    int vertexIdx = toZeroBasedIndex(rawVertex, vertexCount);
    if (vertexIdx < 0 || vertexIdx >= vertexCount) {
      throw new IOException("obj parser: vertex index " + rawVertex + " out of range");
    }

    int normalIdx = MISSING_INDEX;
    if (secondSlash != -1 && secondSlash + 1 < token.length()) {
      String normalPart = token.substring(secondSlash + 1);
      int rawNormal = Integer.parseInt(normalPart);
      normalIdx = toZeroBasedIndex(rawNormal, normalCount);
      if (normalIdx < 0 || normalIdx >= normalCount) {
        throw new IOException("obj parser: normal index " + rawNormal + " out of range");
      }
    }

    return new FaceVertexRef(vertexIdx, normalIdx);
  }

  private static void parseFace(String[] tokens, int vertexCount, int normalCount, List<List<FaceVertexRef>> faces)
      throws IOException {
    if (tokens.length < 4) {
      return;
    }

    List<FaceVertexRef> refs = new ArrayList<>();
    for (int i = 1; i < tokens.length; i++) {
      refs.add(parseFaceToken(tokens[i], vertexCount, normalCount));
    }
    if (refs.size() >= 3) {
      faces.add(refs);
    }
  }

  private static Vector3D faceNormal(Vector3D a, Vector3D b, Vector3D c) {
    return b.sub(a).cross(c.sub(a)).normalize();
  }

  private static List<Vector3D> buildGeneratedNormals(List<Vector3D> vertices, List<List<FaceVertexRef>> faces) {
    List<Vector3D> generated = new ArrayList<>(vertices.size());
    for (int i = 0; i < vertices.size(); i++) {
      generated.add(new Vector3D(0f, 0f, 0f));
    }
    for (List<FaceVertexRef> face : faces) {
      for (int i = 1; i + 1 < face.size(); i++) {
        Vector3D a = vertices.get(face.get(0).vertexIdx());
        Vector3D b = vertices.get(face.get(i).vertexIdx());
        Vector3D c = vertices.get(face.get(i + 1).vertexIdx());
        Vector3D fn = faceNormal(a, b, c);
        generated.set(face.get(0).vertexIdx(), generated.get(face.get(0).vertexIdx()).add(fn));
        generated.set(face.get(i).vertexIdx(), generated.get(face.get(i).vertexIdx()).add(fn));
        generated.set(face.get(i + 1).vertexIdx(), generated.get(face.get(i + 1).vertexIdx()).add(fn));
      }
    }
    for (int i = 0; i < generated.size(); i++) {
      generated.set(i, generated.get(i).normalize());
    }
    return generated;
  }

  private static Vector3D pickNormal(FaceVertexRef ref, List<Vector3D> objNormals, List<Vector3D> generatedNormals,
      Vector3D fallback) {
    if (ref.normalIdx() >= 0) {
      return objNormals.get(ref.normalIdx());
    }
    Vector3D generated = generatedNormals.get(ref.vertexIdx());
    if (generated.lengthSquared() > NORMAL_EPSILON) {
      return generated;
    }
    return fallback;
  }

  private static void triangulateFaces(
      Scene scene,
      List<Vector3D> vertices,
      List<Vector3D> objNormals,
      List<List<FaceVertexRef>> faces,
      List<Vector3D> generatedNormals,
      int materialId) {
    for (List<FaceVertexRef> face : faces) {
      for (int i = 1; i + 1 < face.size(); i++) {
        FaceVertexRef f0 = face.get(0);
        FaceVertexRef f1 = face.get(i);
        FaceVertexRef f2 = face.get(i + 1);

        Vector3D p0 = vertices.get(f0.vertexIdx());
        Vector3D p1 = vertices.get(f1.vertexIdx());
        Vector3D p2 = vertices.get(f2.vertexIdx());
        Vector3D fallback = faceNormal(p0, p1, p2);

        Vector3D n0 = pickNormal(f0, objNormals, generatedNormals, fallback);
        Vector3D n1 = pickNormal(f1, objNormals, generatedNormals, fallback);
        Vector3D n2 = pickNormal(f2, objNormals, generatedNormals, fallback);
        scene.add(new Triangle(p0, p1, p2, n0, n1, n2, materialId));
      }
    }
  }
}
