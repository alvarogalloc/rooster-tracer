
package scene;

import java.io.IOException;
import java.util.Map;
import scene.parsers.DirectionalLightParser;
import scene.parsers.MaterialParser;
import scene.parsers.SphereParser;
import scene.parsers.TriangleParser;
import scene.parsers.ObjParser;

/**
 * Parsers
 */
public class Parsers {

  @FunctionalInterface
  public interface Parser {
    void parse(Scene scene, String[] tokens) throws IOException;
  }

  public static final Map<String, Parser> parsers = Map.of(
      "sphere", SphereParser::parseSphere,
      "triangle", TriangleParser::parseTriangle,
      "obj", ObjParser::parseObj,
      "mat", MaterialParser::parseMaterial,
      "dir_light", DirectionalLightParser::parseDirectionalLight
  );
}
