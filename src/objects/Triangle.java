package objects;

import java.awt.Color;
import java.util.Optional;
import math.Intersection;
import math.Interval;
import math.Matrix3;
import math.Ray;
import math.Vector3D;

public class Triangle  implements  Object3D{
    Vector3D p0;
    Vector3D p1;
    Vector3D p2;
    Color color;
    

    public Triangle(Vector3D p0, Vector3D p1, Vector3D p2, Color color) {
        this.p0 = p0;
        this.p1 = p1;
        this.p2 = p2;
        this.color = color;
    }
    @Override
    public Color getColor() {
        return color;
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
        
        Vector3D normal = col1.cross(col2).normalize();

        return Optional.of(new Intersection(hitPoint, normal, t));
    }
}
