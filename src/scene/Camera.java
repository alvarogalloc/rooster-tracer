// Simple camera:
// - Generates rays for each pixel 
// - lets consumer handle ray tracing and color assignment

package scene;
import math.Ray;
import math.Vector3D;
public class Camera {
  private int width;
  private int height;

  private float fov;

  private Vector3D pos;
  private Vector3D up;
  private Vector3D lookAt;

  private float near;
  private float far;
  public int getWidth() {
    return width;
  }

  public int getHeight() {
    return height;
  }


  public Camera(int width, int height, float fov, Vector3D pos, Vector3D up, Vector3D lookAt, float near, float far) {
    this.width = width;
    this.height = height;
    this.fov = fov;
    this.pos = pos;
    this.up = up;
    this.lookAt = lookAt;
    this.near = near;
    this.far = far;
  }

  @FunctionalInterface
  public interface RayConsumer {
    void accept(Ray ray, int x, int y);
  }

  public void castRays(RayConsumer rayCallback) {
    final Vector3D forward = lookAt.add(pos.mul(-1)).normalize();
    final Vector3D right = forward.cross(up).normalize();
    final Vector3D upVec = right.cross(forward).normalize();

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        // compute ray direction based on camera parameters
        final Ray ray = computeRay(x, y, forward, right, upVec);
        rayCallback.accept(ray, x, y);
      }
    }
  }

  private record NdcCoords(float x, float y) {
  }

  private NdcCoords screenToNDCCoords(int x, int y) {
    final float aspectRatio = (float) width / height;
    return new NdcCoords((2 * ((x + 0.5f) / width) - 1) * (float) Math.tan(fov / 2) * aspectRatio,
        (1 - 2 * ((y + 0.5f) / height)) * (float) Math.tan(fov / 2));
  }

  private Ray computeRay(int x, int y, Vector3D forward, Vector3D right, Vector3D upVec) {
    final var ndc = screenToNDCCoords(x, y);
    Vector3D rayDir = forward.add(right.mul(ndc.x)).add(upVec.mul(ndc.y)).normalize();
    return new Ray(pos, rayDir);
  }

}
