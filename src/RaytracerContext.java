import java.awt.Color;
import java.awt.image.BufferedImage;

public class RaytracerContext {
    private Scene scene;
    private Camera camera;
    private Color bgColor;
    private BufferedImage image;
    private int maxDepth = 5;


    public RaytracerContext(Scene scene, Camera camera, Color bgColor) {
        this.scene = scene;
        this.camera = camera;
        this.bgColor = bgColor;
        this.image = new BufferedImage(camera.getWidth(), camera.getHeight(), BufferedImage.TYPE_INT_RGB);
    }

    public BufferedImage getImage() {
        return image;
    }

    public Scene getScene() {
        return scene;
    }

    public Camera getCamera() {
        return camera;
    }

    public Color getBgColor() {
        return bgColor;
    }

    public int getMaxDepth() {
        return maxDepth;
    }
}
