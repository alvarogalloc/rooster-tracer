package math;

public class Matrix3 {
    Vector3D c0;
    Vector3D c1;
    Vector3D c2;

    public Matrix3(Vector3D c0, Vector3D c1, Vector3D c2) {
        this.c0 = c0;
        this.c1 = c1;
        this.c2 = c2;
    }
    public Vector3D getColumn(int j) {
        switch (j) {
            case 0 -> {
                return c0;
            }
            case 1 -> {
                return c1;
            }
            case 2 -> {
                return c2;
            }
            default -> throw new IndexOutOfBoundsException("Column index must be 0, 1, or 2");
        }
    }

    public float get(int i, int j) {
        Vector3D col = getColumn(j);
        switch (i) {
            case 0 -> {
                return col.getX();
            }
            case 1 -> {
                return col.getY();
            }
            case 2 -> {
                return col.getZ();
            }
            default -> throw new IndexOutOfBoundsException("Row index must be 0, 1, or 2");
        }
    }
    public float determinant() {
        float minor0 = c1.getY() * c2.getZ() - c1.getZ() * c2.getY();
        float minor1 = c0.getY() * c2.getZ() - c0.getZ() * c2.getY();
        float minor2 = c0.getY() * c1.getZ() - c0.getZ() * c1.getY();
        return c0.getX() * minor0 - c1.getX() * minor1 + c2.getX() * minor2;
    }
}
