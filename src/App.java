import java.awt.Color;
import math.*;
import scene.*;

public class App {

  public static void main(String[] args) throws Exception {
    Camera camera = new Camera(
        800, 600,
        (float) Math.toRadians(90),
        new Vector3D(0, 0, 0), // pos
        new Vector3D(0, 1, 0), // up
        new Vector3D(0, 0, -1), // lookAt (looking down the negative Z axis),
        0.01f, // near plane
        1000f // far plane
    );

    // 2. Setup Scene from file
    Scene scene = SceneParser.parseScene("sample.rscn");

    RaytracerContext context = new RaytracerContext(scene, camera, Color.WHITE, 5);

    Raytracer tracer = new Raytracer(context);
    tracer.run("output.png");

    System.out.println("Render complete! Saved to output.png");
  }
}
