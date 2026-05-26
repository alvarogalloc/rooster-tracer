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
import scene.Light;
import scene.Material;

public class Raytracer {
  private static final float SHADOW_BIAS = 1e-3f;
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
    for (Object3D obj : context.getScene().getObjects()) {
      Optional<Intersection> hit = obj.isHit(ray, tRange);
      if (hit.isPresent()) {
        if (closestHit == null || hit.get().getT() < closestHit.getT()) {
          closestHit = hit.get();
        }
      }
    }
    if (closestHit == null) {
      return context.getBgColor();
    }
    return shadeHit(closestHit, ray.getDir().mul(-1f));
  }

  private static int toByte(float channel) {
    float clamped = Math.max(0f, Math.min(1f, channel));
    return Math.round(clamped * 255f);
  }

  private Color shadeHit(Intersection hit, Vector3D viewDir) {
    int materialId = hit.getMaterialId();
    if (materialId < 0 || materialId >= context.getScene().getMaterials().size()) {
      throw new IllegalStateException("invalid material id " + materialId + " for hit event");
    }
    Material material = context.getScene().getMaterials().get(materialId);

    Vector3D result = material.getAmbient();
    for (Light light : context.getScene().getLights()) {
      if (isInShadow(light, hit)) {
        continue;
      }
      result = result.add(light.shade(material, hit, viewDir));
    }
    return new Color(toByte(result.getX()), toByte(result.getY()), toByte(result.getZ()));
  }

  private boolean isInShadow(Light light, Intersection hit) {
    Light.ShadowRay shadow = light.shadowRay(hit);
    if (shadow.maxDistance() <= SHADOW_BIAS) {
      return false;
    }
    float maxDistance = Float.isInfinite(shadow.maxDistance())
        ? context.getCamera().getFarPlane()
        : shadow.maxDistance() - SHADOW_BIAS;
    if (maxDistance <= SHADOW_BIAS) {
      return false;
    }
    Interval tRange = new Interval(SHADOW_BIAS, maxDistance);
    for (Object3D obj : context.getScene().getObjects()) {
      if (obj.isHit(shadow.ray(), tRange).isPresent()) {
        return true;
      }
    }
    return false;
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
