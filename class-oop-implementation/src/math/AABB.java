package math;

public class AABB {
    private final Vector3D min;
    private final Vector3D max;

    public AABB(Vector3D min, Vector3D max) {
        this.min = min;
        this.max = max;
    }

    public static AABB empty() {
        float inf = Float.POSITIVE_INFINITY;
        return new AABB(
            new Vector3D(inf, inf, inf),
            new Vector3D(-inf, -inf, -inf)
        );
    }

    public Vector3D getMin() {
        return min;
    }

    public Vector3D getMax() {
        return max;
    }

    public boolean isEmpty() {
        return min.getX() > max.getX();
    }

    public AABB extend(Vector3D p) {
        if (isEmpty()) {
            return new AABB(p, p);
        }
        return new AABB(
            new Vector3D(Math.min(min.getX(), p.getX()), Math.min(min.getY(), p.getY()), Math.min(min.getZ(), p.getZ())),
            new Vector3D(Math.max(max.getX(), p.getX()), Math.max(max.getY(), p.getY()), Math.max(max.getZ(), p.getZ()))
        );
    }

    public AABB extend(AABB other) {
        if (other.isEmpty()) {
            return this;
        }
        if (isEmpty()) {
            return other;
        }
        return new AABB(
            new Vector3D(Math.min(min.getX(), other.min.getX()), Math.min(min.getY(), other.min.getY()), Math.min(min.getZ(), other.min.getZ())),
            new Vector3D(Math.max(max.getX(), other.max.getX()), Math.max(max.getY(), other.max.getY()), Math.max(max.getZ(), other.max.getZ()))
        );
    }

    public float surfaceArea() {
        if (isEmpty()) {
            return 0f;
        }
        float dx = max.getX() - min.getX();
        float dy = max.getY() - min.getY();
        float dz = max.getZ() - min.getZ();
        return 2.0f * (dx * dy + dx * dz + dy * dz);
    }

    public boolean isHit(Ray ray, Interval tRange) {
        Vector3D dir = ray.getDir();
        Vector3D pos = ray.getPos();

        float invDirX = 1.0f / dir.getX();
        float invDirY = 1.0f / dir.getY();
        float invDirZ = 1.0f / dir.getZ();

        float tNearX = (min.getX() - pos.getX()) * invDirX;
        float tFarX = (max.getX() - pos.getX()) * invDirX;
        float tMinX = Math.min(tNearX, tFarX);
        float tMaxX = Math.max(tNearX, tFarX);

        float tNearY = (min.getY() - pos.getY()) * invDirY;
        float tFarY = (max.getY() - pos.getY()) * invDirY;
        float tMinY = Math.min(tNearY, tFarY);
        float tMaxY = Math.max(tNearY, tFarY);

        float tNearZ = (min.getZ() - pos.getZ()) * invDirZ;
        float tFarZ = (max.getZ() - pos.getZ()) * invDirZ;
        float tMinZ = Math.min(tNearZ, tFarZ);
        float tMaxZ = Math.max(tNearZ, tFarZ);

        float tEnter = Math.max(tMinX, Math.max(tMinY, tMinZ));
        float tExit = Math.min(tMaxX, Math.min(tMaxY, tMaxZ));

        float tStart = Math.max(tEnter, tRange.getMin());
        float tEnd = Math.min(tExit, tRange.getMax());

        return tStart <= tEnd;
    }
}
