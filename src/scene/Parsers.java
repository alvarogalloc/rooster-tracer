
package scene;

import java.io.IOException;
import java.util.Map;
import scene.parsers.DirectionalLightParser;
import scene.parsers.CameraParser;
import scene.parsers.BackgroundParser;
import scene.parsers.FovParser;
import scene.parsers.MaterialParser;
import scene.parsers.MaxDepthParser;
import scene.parsers.PlaneParser;
import scene.parsers.PointLightParser;
import scene.parsers.SphereParser;
import scene.parsers.TriangleParser;
import scene.parsers.ObjParser;
import scene.parsers.ViewportParser;

/**
 * Parsers
 */
public class Parsers {

  @FunctionalInterface
  public interface Parser {
    void parse(Scene scene, String[] tokens) throws IOException;
  }

  public static final Map<String, Parser> parsers = Map.ofEntries(
      Map.entry("sphere", SphereParser::parseSphere),
      Map.entry("triangle", TriangleParser::parseTriangle),
      Map.entry("obj", ObjParser::parseObj),
      Map.entry("plane", PlaneParser::parsePlane),
      Map.entry("mat", MaterialParser::parseMaterial),
      Map.entry("dir_light", DirectionalLightParser::parseDirectionalLight),
      Map.entry("point_light", PointLightParser::parsePointLight),
      Map.entry("viewport", ViewportParser::parseViewport),
      Map.entry("camera", CameraParser::parseCamera),
      Map.entry("fov", FovParser::parseFov),
      Map.entry("background", BackgroundParser::parseBackground),
      Map.entry("max_depth", MaxDepthParser::parseMaxDepth)
  );
}
