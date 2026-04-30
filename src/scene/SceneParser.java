package scene;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class SceneParser {
  private static final String fileToken = "ROOSTERSCENEV1";

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
    Parsers.Parser parser = Parsers.parsers.get(type);
    if (parser == null) {
      System.err.println("unknown type of object found: " + type);
      return;
    }
    parser.parse(scene, tokens);
  }

}
