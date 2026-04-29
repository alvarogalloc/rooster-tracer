package math;
public class Intersection {
    final private Vector3D point;
    final private Vector3D normal;
    final private float t;
    
    public Intersection(Vector3D point, Vector3D normal, float t) {
        this.point = point;
        this.normal = normal;
        this.t = t;
    }

    public Vector3D getPoint() {
        return point;
    }

    public Vector3D getNormal() {
        return normal;
    }

    public float getT() {
        return t;
    }

}
