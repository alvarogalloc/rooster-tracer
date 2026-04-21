// Simple camera:
// - Generates rays for each pixel 
// - lets consumer handle ray tracing and color assignment

public class Camera {
  private int width;
  private int height;

  public int getWidth() {
    return width;
  }

  public int getHeight() {
    return height;
  }

  private float fov;

  private Vector3D pos;
  private Vector3D up;
  private Vector3D lookAt;

  public Camera(int width, int height, float fov, Vector3D pos, Vector3D up, Vector3D lookAt) {
    this.width = width;
    this.height = height;
    this.fov = fov;
    this.pos = pos;
    this.up = up;
    this.lookAt = lookAt;
  }

  @FunctionalInterface
  public interface RayConsumer {
    void accept(Ray ray, int x, int y);
  }

  public void castRays(RayConsumer rayCallback) {
    Vector3D forward = lookAt.add(pos.mul(-1)).normalize();
    Vector3D right = forward.cross(up).normalize();
    Vector3D upVec = right.cross(forward).normalize();

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        // compute ray direction based on camera parameters
        Ray ray = computeRay(x, y, forward, right, upVec);
        rayCallback.accept(ray, x, y);
      }
    }
  }

  private Ray computeRay(int x, int y, Vector3D forward, Vector3D right, Vector3D upVec) {
    float aspectRatio = (float) width / height;
    float px = (2 * ((x + 0.5f) / width) - 1) * (float) Math.tan(fov / 2) * aspectRatio;
    float py = (1 - 2 * ((y + 0.5f) / height)) * (float) Math.tan(fov / 2);

    Vector3D rayDir = forward.add(right.mul(px)).add(upVec.mul(py)).normalize();
    return new Ray(pos, rayDir);
  }

}
