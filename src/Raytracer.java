import java.awt.Color;
import java.io.File;
import java.io.IOException;
import java.util.Optional;
import javax.imageio.ImageIO;
import math.Intersection;
import math.Interval;
import math.Ray;
import objects.Object3D;

public class Raytracer {
  private RaytracerContext context;

  public Raytracer(RaytracerContext context) {
    this.context = context;
  }

  public void run(String outputPath) {
    render();
    saveImage(outputPath);
  }

  private void saveImage(String outputPath)  {
    File outputFile = new File(outputPath);
    try {
      ImageIO.write(context.getImage(), "png", outputFile);
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  private Color traceRay(Ray ray, int depth) {
    if (depth <= 0) {
      return context.getBgColor();
    }
    Intersection closestHit = null;
    Interval tRange = new Interval(this.context.getCamera().getNearPlane(), this.context.getCamera().getFarPlane());
    for (Object3D obj : context.getScene().getObjects()) {
      Optional<Intersection> hit = obj.isHit(ray,tRange);
      if (hit.isPresent()) {
        if (closestHit == null || hit.get().getT() < closestHit.getT()) {
          closestHit = hit.get();
        }
      }
    }
    if (closestHit != null) {
      return closestHit.getObj().getColor();
    } else {
      return context.getBgColor();
    }
  }

  public void render() {
    this.context.getCamera().castRays((ray, x, y) -> {
      Color color = traceRay(ray, this.context.getMaxDepth());
      // Set the color of the corresponding pixel in the image
      this.context.getImage().setRGB(x, y, color.getRGB());
    });
  }

}
