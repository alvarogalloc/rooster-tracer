import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;
import scene.Camera;
import scene.Scene;
import scene.SceneParser;

public class RenderTestScenes {
  private static final String DEFAULT_SCENES_DIR = "test-scenes";
  private static final String DEFAULT_OUTPUT_DIR = "test-imgs";

  private static List<Path> sceneFiles(Path scenesDir) throws IOException {
    try (var files = Files.list(scenesDir)) {
      return files
          .filter(path -> path.getFileName().toString().endsWith(".rscn"))
          .sorted(Comparator.comparing(path -> path.getFileName().toString()))
          .collect(Collectors.toList());
    }
  }

  private static void renderScene(Path scenePath, Path outputPath) throws Exception {
    Scene scene = SceneParser.parseScene(scenePath.toString());
    String validationError = scene.validateForRender();
    if (validationError != null) {
      throw new IllegalStateException("scene validation failed (" + scenePath.getFileName() + "): " + validationError);
    }
    Camera camera = new Camera(
        scene.getImageWidth(),
        scene.getImageHeight(),
        scene.getFov(),
        scene.getCameraPos(),
        scene.getCameraUp(),
        scene.getCameraLookAt(),
        scene.getNearPlane(),
        scene.getFarPlane());
    RaytracerContext context = new RaytracerContext(scene, camera, scene.getBackground(), scene.getMaxDepth());
    Raytracer tracer = new Raytracer(context);
    tracer.run(outputPath.toString());
  }

  public static void main(String[] args) throws Exception {
    Path scenesDir = args.length >= 1 ? Path.of(args[0]) : Path.of(DEFAULT_SCENES_DIR);
    Path outputDir = args.length >= 2 ? Path.of(args[1]) : Path.of(DEFAULT_OUTPUT_DIR);

    if (!Files.isDirectory(scenesDir)) {
      throw new IllegalStateException("scenes directory not found: " + scenesDir.toAbsolutePath());
    }
    Files.createDirectories(outputDir);

    List<Path> scenes = sceneFiles(scenesDir);
    if (scenes.isEmpty()) {
      throw new IllegalStateException("no .rscn files found in: " + scenesDir.toAbsolutePath());
    }

    for (Path scenePath : scenes) {
      String baseName = scenePath.getFileName().toString().replaceFirst("\\.rscn$", "");
      Path outputPath = outputDir.resolve(baseName + ".png");
      renderScene(scenePath, outputPath);
      System.out.println("rendered test: " + outputPath);
    }
    System.out.println("done: rendered " + scenes.size() + " scenes");
  }
}
