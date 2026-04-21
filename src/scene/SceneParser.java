package scene;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Optional;
import objects.Object3D;

public class SceneParser {
  private static String fileToken = "ROOSTERSCENEV1";

  private static boolean isValidScene(String header) {

    return header != null && header.trim().equals(fileToken);
  }

  public static Scene parseScene(String filePath) throws IOException {
    Scene scene = new Scene();

    try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
      String header = reader.readLine();
      if (!isValidScene(header)) {
        throw new IOException("header is not there, expected: " + fileToken + ", recieved: " + header);
      }

      String line;
      while ((line = reader.readLine()) != null) {
        line = line.trim();
        if (line.isEmpty() || line.startsWith("#")) { // Skip empty lines and comments
          continue;
        }
        parseLine(scene, line);
      }
    }

    return scene;
  }

  private static void parseLine(Scene scene, String line) throws IOException {
    String[] tokens = line.split("\\s+");
    String type = tokens[0].toLowerCase();
    Optional<Object3D> obj = Parsers.parsers.get(type).parse(tokens);
    if (obj.isEmpty()) {
      // skip
      return;
    }
    scene.add(obj.get());
  }

}
