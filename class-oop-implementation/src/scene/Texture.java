package scene;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import javax.imageio.ImageIO;
import math.Vector2D;
import math.Vector3D;

public class Texture {
    private final int width;
    private final int height;
    private final Vector3D[] pixels;

    private Texture(int width, int height, Vector3D[] pixels) {
        this.width = width;
        this.height = height;
        this.pixels = pixels;
    }

    public static Texture load(String path) throws IOException {
        BufferedImage image = ImageIO.read(new File(path));
        if (image == null) {
            throw new IOException("texture load failed: " + path);
        }
        int width = image.getWidth();
        int height = image.getHeight();
        Vector3D[] pixels = new Vector3D[width * height];

        for (int y = 0; y < height; y++) {
            int srcY = height - 1 - y;
            for (int x = 0; x < width; x++) {
                int rgb = image.getRGB(x, srcY);
                int r = (rgb >> 16) & 0xFF;
                int g = (rgb >> 8) & 0xFF;
                int b = rgb & 0xFF;
                Vector3D srgb = new Vector3D(r / 255.99f, g / 255.99f, b / 255.99f);
                pixels[y * width + x] = Gamma.srgbToLinear(srgb);
            }
        }
        return new Texture(width, height, pixels);
    }

    private static float wrapUv(float v) {
        float wrapped = (float) (v - Math.floor(v));
        return wrapped < 0f ? wrapped + 1f : wrapped;
    }

    private int index(int x, int y) {
        return y * width + x;
    }

    public Vector3D sample(Vector2D uv) {
        if (pixels.length == 0 || width <= 0 || height <= 0) {
            return new Vector3D(1f, 1f, 1f);
        }
        float u = wrapUv(uv.getX());
        float v = wrapUv(uv.getY());
        int x = Math.min(Math.max((int) (u * width), 0), width - 1);
        int y = Math.min(Math.max((int) (v * height), 0), height - 1);
        return pixels[index(x, y)];
    }
}
