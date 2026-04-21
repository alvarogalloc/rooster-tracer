
package scene;

import java.util.Map;
import java.util.Optional;

import objects.Object3D;
import scene.parsers.SphereParser;

/**
 * Parsers
 */
public class Parsers {

  @FunctionalInterface
  public interface Parser {
    Optional<Object3D> parse(String[] tokens);
  }

  public static Map<String, Parser> parsers = Map.of("sphere", SphereParser::parseSphere);
}
