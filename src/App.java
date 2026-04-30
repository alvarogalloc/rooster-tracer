import java.awt.Color;
import math.*;
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

    Camera camera = new Camera(
        400, 400,
        (float) Math.toRadians(45),
        new Vector3D(0.4f, 0.65f, 1.4f), // pos
        new Vector3D(0, 1, 0), // up
        new Vector3D(-0.037f, 0.458f, 0.192f), // lookAt
        0.001f, // near plane
        20f // far plane
    );

    try {
      Scene scene = SceneParser.parseScene(scenePath);
      RaytracerContext context = new RaytracerContext(scene, camera, new Color(10, 32, 90), 5);
      Raytracer tracer = new Raytracer(context);
      tracer.run(outputPath);
      System.out.println(outputPath + " rendered correctly!!!");
    } catch (Exception e) {
      System.err.println("exception caught (rooster-tracer)!, exiting...\n[reason] -> \"" + e.getMessage() + "\"");
    }
  }
}
