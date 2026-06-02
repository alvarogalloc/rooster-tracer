package objects;

import java.util.Optional;
import math.Intersection;
import math.Interval;
import math.Matrix3;
import math.Ray;
import math.Vector2D;
import math.Vector3D;

public class Triangle implements Object3D {
    private static final float NORMAL_EPSILON = 1e-12f;
    private final Vector3D p0;
    private final Vector3D p1;
    private final Vector3D p2;
    private final Vector3D n0;
    private final Vector3D n1;
    private final Vector3D n2;
    private final Vector2D uv0;
    private final Vector2D uv1;
    private final Vector2D uv2;
    private final boolean hasVertexNormals;
    private final boolean hasVertexUvs;
    private final int materialId;

    public Triangle(Vector3D p0, Vector3D p1, Vector3D p2, int materialId) {
        this(
            p0,
            p1,
            p2,
            new Vector3D(0, 0, 0),
            new Vector3D(0, 0, 0),
            new Vector3D(0, 0, 0),
            new Vector2D(0, 0),
            new Vector2D(0, 0),
            new Vector2D(0, 0),
            false,
            false,
            materialId
        );
    }

    public Triangle(Vector3D p0, Vector3D p1, Vector3D p2, Vector3D n0, Vector3D n1, Vector3D n2, int materialId) {
        this(
            p0,
            p1,
            p2,
            n0,
            n1,
            n2,
            new Vector2D(0, 0),
            new Vector2D(0, 0),
            new Vector2D(0, 0),
            true,
            false,
            materialId
        );
    }

    public Triangle(
        Vector3D p0,
        Vector3D p1,
        Vector3D p2,
        Vector3D n0,
        Vector3D n1,
        Vector3D n2,
        Vector2D uv0,
        Vector2D uv1,
        Vector2D uv2,
        int materialId
    ) {
        this(p0, p1, p2, n0, n1, n2, uv0, uv1, uv2, true, true, materialId);
    }

    private Triangle(
        Vector3D p0,
        Vector3D p1,
        Vector3D p2,
        Vector3D n0,
        Vector3D n1,
        Vector3D n2,
        Vector2D uv0,
        Vector2D uv1,
        Vector2D uv2,
        boolean hasVertexNormals,
        boolean hasVertexUvs,
        int materialId
    ) {
        this.p0 = p0;
        this.p1 = p1;
        this.p2 = p2;
        this.n0 = n0;
        this.n1 = n1;
        this.n2 = n2;
        this.uv0 = uv0;
        this.uv1 = uv1;
        this.uv2 = uv2;
        this.hasVertexNormals = hasVertexNormals;
        this.hasVertexUvs = hasVertexUvs;
        this.materialId = materialId;
    }

    @Override
    public Optional<Intersection> isHit(Ray ray, Interval tRange) {
        // col0 = -ray.direction
        Vector3D col0 = ray.getDir().mul(-1);
        // col1 = p1 - p0
        Vector3D col1 = p1.add(p0.mul(-1));
        // col2 = p2 - p0
        Vector3D col2 = p2.add( p0.mul(-1));
        // col3 = ray.origin - p0
        Vector3D col3 = ray.getPos().add(p0.mul(-1));

        // System matrix: M = [col0 | col1 | col2]
        Matrix3 m = new Matrix3(col0, col1, col2);
        float d0 = m.determinant();

        if (Math.abs(d0) < 1e-4) {
            return Optional.empty();
        }

        float dt = new Matrix3(col3, col1, col2).determinant();
        float t = dt / d0;
        if (t < 0 || !tRange.contains(t)) {
            return Optional.empty(); // Hit is behind the ray origin
        }

        float du = new Matrix3(col0, col3, col2).determinant();
        float u = du / d0;
        if (u < 0 || u > 1) {
            return Optional.empty();
        }

        float dv = new Matrix3(col0, col1, col3).determinant();
        float v = dv / d0;
        if (v < 0 || v > (1 - u)) {
            return Optional.empty();
        }

        // Intersection confirmed
        Vector3D hitPoint = ray.at(t);
        
        final float baryU = u;
        final float baryV = v;
        final float baryW = 1f - baryU - baryV;
        Vector3D faceNormal = col1.cross(col2).normalize();
        Vector3D normal = faceNormal;
        if (hasVertexNormals) {
            normal = n0.mul(baryW).add(n1.mul(baryU)).add(n2.mul(baryV)).normalize();
            if (normal.lengthSquared() <= NORMAL_EPSILON) {
                normal = faceNormal;
            }
        }
        if (normal.dot(ray.getDir()) > 0f) {
            normal = normal.mul(-1f);
        }

        Vector2D uv = new Vector2D(0f, 0f);
        Vector3D dpdu = new Vector3D(1f, 0f, 0f);
        Vector3D dpdv = new Vector3D(0f, 1f, 0f);
        if (hasVertexUvs) {
            uv = uv0.mul(baryW).add(uv1.mul(baryU)).add(uv2.mul(baryV));
            Vector2D duv1 = uv1.sub(uv0);
            Vector2D duv2 = uv2.sub(uv0);
            float det = duv1.getX() * duv2.getY() - duv1.getY() * duv2.getX();
            if (Math.abs(det) > 1e-8f) {
                float invDet = 1f / det;
                dpdu = col1.mul(duv2.getY()).sub(col2.mul(duv1.getY())).mul(invDet);
                dpdv = col2.mul(duv1.getX()).sub(col1.mul(duv2.getX())).mul(invDet);
            } else {
                Vector3D axis = Math.abs(normal.getX()) > 0.9f ? new Vector3D(0f, 1f, 0f) : new Vector3D(1f, 0f, 0f);
                dpdu = normal.cross(axis).normalize();
                dpdv = normal.cross(dpdu).normalize();
            }
        }

        return Optional.of(new Intersection(hitPoint, normal, uv, dpdu, dpdv, t, materialId));
    }

    public Vector3D getP0() { return p0; }
    public Vector3D getP1() { return p1; }
    public Vector3D getP2() { return p2; }
    public Vector3D getN0() { return n0; }
    public Vector3D getN1() { return n1; }
    public Vector3D getN2() { return n2; }
    public Vector2D getUv0() { return uv0; }
    public Vector2D getUv1() { return uv1; }
    public Vector2D getUv2() { return uv2; }
    public boolean hasVertexNormals() { return hasVertexNormals; }
    public boolean hasVertexUvs() { return hasVertexUvs; }
    public int getMaterialId() { return materialId; }
}
