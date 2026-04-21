import java.awt.Color;
import java.util.Optional;
public interface    Object3D {
    public Color getColor();

    public   Optional<Intersection> isHit(Ray ray); 
}
