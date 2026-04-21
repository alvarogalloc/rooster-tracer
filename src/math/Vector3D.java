
package math;

public class Vector3D {
    private float x;
    private float y;
    private float z;

    public Vector3D(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public float getX() {
        return x;
    }

    public void setX(float x) {
        this.x = x;
    }

    public float getY() {
        return y;
    }

    public void setY(float y) {
        this.y = y;
    }

    public float getZ() {
        return z;
    }

    public void setZ(float z) {
        this.z = z;
    }

    public Vector3D add(Vector3D other) {
        return new Vector3D(this.x + other.x, this.y + other.y, this.z + other.z);
    }

    public Vector3D mul(float t) {
        return new Vector3D(this.x * t, this.y * t, this.z * t);
    }

    public float dot(Vector3D other) {
        return this.x * other.x + this.y * other.y + this.z * other.z;
    }
    public Vector3D cross(Vector3D other) {
        return new Vector3D(
            this.y * other.z - this.z * other.y,
            this.z * other.x - this.x * other.z,
            this.x * other.y - this.y * other.x
        );
    }

    public Vector3D vec_mul(Vector3D other) {
        return new Vector3D(this.x * other.x, this.y * other.y, this.z * other.z);
    }
    public Vector3D normalize() {
        float length = (float) Math.sqrt(x * x + y * y + z * z);
        if (length == 0) {
            return new Vector3D(0, 0, 0); // no division by zero
        }
        return this.mul(1 / length);
    }
}
