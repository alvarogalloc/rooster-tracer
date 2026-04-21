import java.awt.Color;
import java.util.Optional;

public class Sphere implements Object3D {
    private Vector3D center;
    private float radius;
    private Color color;

    public Sphere(Vector3D center, float radius, Color color) {
        this.center = center;
        this.radius = radius;
        this.color = color;
    }

    public Vector3D getCenter() {
        return center;
    }

    public float getRadius() {
        return radius;
    }
    @Override
    public Color getColor() {
        return color;
    }

    @Override
    public Optional<Intersection> isHit(Ray ray) {
        Vector3D oc = ray.getPos().add(center.mul(-1));
        float a = ray.getDir().dot(ray.getDir());
        float b = 2.0f * oc.dot(ray.getDir());
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0) {
            return Optional.empty();
        } else {
            float t = (-b - (float) Math.sqrt(discriminant)) / (2.0f * a);
            if (t > 0) {
                return Optional.of(new Intersection(this, t));
            } else {
                return Optional.empty();
            }
        }
    }

}
