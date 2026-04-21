import java.awt.Color;
import java.awt.image.BufferedImage;

import scene.Camera;
import scene.Scene;

public class RaytracerContext {
    private Scene scene;
    private Camera camera;
    private Color bgColor;
    private BufferedImage image;
    private int maxDepth;


    public RaytracerContext(Scene scene, Camera camera, Color bgColor, int maxDepth) {
        this.scene = scene;
        this.camera = camera;
        this.bgColor = bgColor;
        this.image = new BufferedImage(camera.getWidth(), camera.getHeight(), BufferedImage.TYPE_INT_RGB);
        this.maxDepth = maxDepth;
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
