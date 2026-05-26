import scene.*;

public class App {

  public static void main(String[] args) {
    if (args.length != 2) {
      System.err.println(
          "please set the scene to render and the output img\n usage: java src/App.java scene.rscn output.png");
      return;
    }

    String scenePath = args[0];
    String outputPath = args[1];

    try {
      Scene scene = SceneParser.parseScene(scenePath);
      String validationError = scene.validateForRender();
      if (validationError != null) {
        throw new IllegalStateException("scene validation failed: " + validationError);
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
      tracer.run(outputPath);
      System.out.println(outputPath + " rendered correctly!!!");
    } catch (Exception e) {
      System.err.println("exception caught (rooster-tracer)!, exiting...\n[reason] -> \"" + e.getMessage() + "\"");
    }
  }
}
