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
import scene.Gamma;
import scene.Light;
import scene.Material;
import scene.Texture;

public class Raytracer {
  final private RaytracerContext context;
  final private Vector3D linearBg;

  public Raytracer(RaytracerContext context) {
    this.context = context;
    java.awt.Color bg = context.getBgColor();
    this.linearBg = new Vector3D(bg.getRed() / 255.99f, bg.getGreen() / 255.99f, bg.getBlue() / 255.99f);
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
      return toColor(linearBg);
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
      return toColor(linearBg);
    }
    return shadeHit(closestHit, ray.getDir().mul(-1f));
  }

  private static int toByte(float channel) {
    float clamped = Math.max(0f, Math.min(1f, channel));
    return (int) (clamped * 255.99f);
  }

  private static Vector3D safeNormalize(Vector3D v) {
    if (v.lengthSquared() <= 1e-8f) {
      return new Vector3D(0f, 0f, 0f);
    }
    return v.normalize();
  }

  private Color toColor(Vector3D linear) {
    Vector3D encoded = Gamma.linearToSrgb(linear);
    return new Color(toByte(encoded.getX()), toByte(encoded.getY()), toByte(encoded.getZ()));
  }

  private Color shadeHit(Intersection hit, Vector3D viewDir) {
    int materialId = hit.getMaterialId();
    if (materialId < 0 || materialId >= context.getScene().getMaterials().size()) {
      throw new IllegalStateException("invalid material id " + materialId + " for hit event");
    }
    Material material = context.getScene().getMaterials().get(materialId);
    Intersection shadedHit = hit;
    Material shadedMaterial = material;
    if (material.getTextureId() != null) {
      int texId = material.getTextureId();
      if (texId >= 0 && texId < context.getScene().getTextures().size()) {
        Texture tex = context.getScene().getTextures().get(texId);
        Vector3D texColor = tex.sample(hit.getUv());
        shadedMaterial = new Material(
            material.getAmbient().vec_mul(texColor),
            material.getDiffuse().vec_mul(texColor),
            material.getSpecular(),
            material.getShininess(),
            material.getTextureId(),
            material.getNormalMapId());
      }
    }
    if (material.getNormalMapId() != null) {
      int mapId = material.getNormalMapId();
      if (mapId >= 0 && mapId < context.getScene().getTextures().size()) {
        Texture map = context.getScene().getTextures().get(mapId);
        Vector3D normColor = map.sample(hit.getUv());
        Vector3D mapN = new Vector3D(normColor.getX() * 2f - 1f, normColor.getY() * 2f - 1f,
            normColor.getZ() * 2f - 1f);

        Vector3D n = safeNormalize(hit.getNormal());
        Vector3D dpdu = hit.getDpdu();
        Vector3D dpdv = hit.getDpdv();
        Vector3D t = safeNormalize(dpdu.sub(n.mul(n.dot(dpdu))));
        Vector3D b = n.cross(t);
        if (b.dot(dpdv) < 0f) {
          b = b.mul(-1f);
        }
        Vector3D mappedNormal = safeNormalize(
            t.mul(mapN.getX()).add(b.mul(mapN.getY())).add(n.mul(mapN.getZ())));
        shadedHit = shadedHit.withNormal(mappedNormal);
      }
    }

    Vector3D result = shadedMaterial.getAmbient();
    for (Light light : context.getScene().getLights()) {
      if (isInShadow(light, shadedHit)) {
        continue;
      }
      result = result.add(light.shade(shadedMaterial, shadedHit, viewDir));
    }
    return toColor(result);
  }

  private boolean isInShadow(Light light, Intersection hit) {
    Light.ShadowRay shadow = light.shadowRay(hit);
    if (shadow.maxDistance() <= Light.SHADOW_BIAS) {
      return false;
    }
    float maxDistance = Float.isInfinite(shadow.maxDistance())
        ? context.getCamera().getFarPlane()
        : shadow.maxDistance() - Light.SHADOW_BIAS;
    if (maxDistance <= Light.SHADOW_BIAS) {
      return false;
    }
    Interval tRange = new Interval(Light.SHADOW_BIAS, maxDistance);
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
