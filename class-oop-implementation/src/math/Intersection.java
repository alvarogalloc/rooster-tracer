package math;
public class Intersection {
    final private Vector3D point;
    final private Vector3D normal;
    final private Vector2D uv;
    final private Vector3D dpdu;
    final private Vector3D dpdv;
    final private float t;
    final private int materialId;
    
    public Intersection(Vector3D point, Vector3D normal, float t, int materialId) {
        this(point, normal, new Vector2D(0f, 0f), new Vector3D(1f, 0f, 0f), new Vector3D(0f, 1f, 0f), t, materialId);
    }

    public Intersection(
        Vector3D point,
        Vector3D normal,
        Vector2D uv,
        Vector3D dpdu,
        Vector3D dpdv,
        float t,
        int materialId
    ) {
        this.point = point;
        this.normal = normal;
        this.uv = uv;
        this.dpdu = dpdu;
        this.dpdv = dpdv;
        this.t = t;
        this.materialId = materialId;
    }

    public Vector3D getPoint() {
        return point;
    }

    public Vector3D getNormal() {
        return normal;
    }

    public Vector2D getUv() {
        return uv;
    }

    public Vector3D getDpdu() {
        return dpdu;
    }

    public Vector3D getDpdv() {
        return dpdv;
    }

    public float getT() {
        return t;
    }

    public int getMaterialId() {
        return materialId;
    }

    public Intersection withNormal(Vector3D newNormal) {
        return new Intersection(point, newNormal, uv, dpdu, dpdv, t, materialId);
    }

}
