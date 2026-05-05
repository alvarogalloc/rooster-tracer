import java.awt.Color;
import java.io.File;
import java.io.IOException;
import java.util.Optional;
import javax.imageio.ImageIO;
import math.Intersection;
import math.Interval;
import math.Ray;
import math.Vector3D;
import objects.Object3D;
import scene.DirectionalLight;

public class Raytracer {
  final private RaytracerContext context;

  public Raytracer(RaytracerContext context) {
    this.context = context;
  }

  public void run(String outputPath) {
    render();
    saveImage(outputPath);
  }

  private void saveImage(String outputPath) {
    File outputFile = new File(outputPath);
    try {
      ImageIO.write(context.getImage(), "png", outputFile);
    } catch (IOException e) {
      System.err.println("Error saving image: " + e.getMessage());
    }
  }

  private Color traceRay(Ray ray, int depth) {
    if (depth <= 0) {
      return context.getBgColor();
    }
    Intersection closestHit = null;
    Interval tRange = new Interval(this.context.getCamera().getNearPlane(), this.context.getCamera().getFarPlane());
    Object3D hitObject = null;
    for (Object3D obj : context.getScene().getObjects()) {
      Optional<Intersection> hit = obj.isHit(ray, tRange);
      if (hit.isPresent()) {
        if (closestHit == null || hit.get().getT() < closestHit.getT()) {
          closestHit = hit.get();
          hitObject = obj;
        }
      }
    }
    if (closestHit == null || hitObject == null) {
      return context.getBgColor();
    }
    DirectionalLight sun = context.getScene().getLights().get(0);
    return shadeFlat(hitObject.getColor(), closestHit, sun);
  }

  private static float toUnit(int channel) {
    return channel / 255.99f;
  }

  private static int toByte(float channel) {
    float clamped = Math.max(0f, Math.min(1f, channel));
    return Math.round(clamped * 255f);
  }

  private Color shadeFlat(Color albedo, Intersection hit, DirectionalLight light) {
    Vector3D n = hit.getNormal().normalize();
    Vector3D lightDir = light.getDir().mul(-1f);
    float lambert = Math.max(0f, n.dot(lightDir));

    float radianceR = toUnit(light.getColor().getRed()) * light.getIntensity();
    float radianceG = toUnit(light.getColor().getGreen()) * light.getIntensity();
    float radianceB = toUnit(light.getColor().getBlue()) * light.getIntensity();

    float shadedR = radianceR * toUnit(albedo.getRed()) * lambert;
    float shadedG = radianceG * toUnit(albedo.getGreen()) * lambert;
    float shadedB = radianceB * toUnit(albedo.getBlue()) * lambert;
    return new Color(toByte(shadedR), toByte(shadedG), toByte(shadedB));
  }

  public void render() {
    if (this.context.getScene().getLights().isEmpty()) {
      throw new IllegalStateException("you should have at least one light!!");
    }
    this.context.getCamera().castRays((ray, x, y) -> {
      Color color = traceRay(ray, this.context.getMaxDepth());
      // Set the color of the corresponding pixel in the image
      this.context.getImage().setRGB(x, y, color.getRGB());
    });
  }

}
