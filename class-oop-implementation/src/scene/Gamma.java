package scene;

import math.Vector3D;

public final class Gamma {
    private static final float LINEAR_THRESH = 0.04045f;
    private static final float LINEAR_SCALE = 12.92f;
    private static final float GAMMA_OFFSET = 0.055f;
    private static final float GAMMA_SCALE = 1.055f;
    private static final float GAMMA_EXP = 2.4f;
    private static final float LINEAR_THRESH_INV = 0.0031308f;

    private Gamma() {
    }

    public static float srgbToLinear(float v) {
        if (v <= LINEAR_THRESH) {
            return v / LINEAR_SCALE;
        }
        return (float) Math.pow((v + GAMMA_OFFSET) / GAMMA_SCALE, GAMMA_EXP);
    }

    public static Vector3D srgbToLinear(Vector3D v) {
        return new Vector3D(srgbToLinear(v.getX()), srgbToLinear(v.getY()), srgbToLinear(v.getZ()));
    }

    public static float linearToSrgb(float v) {
        float clamped = Math.max(0f, Math.min(1f, v));
        if (clamped <= LINEAR_THRESH_INV) {
            return clamped * LINEAR_SCALE;
        }
        return GAMMA_SCALE * (float) Math.pow(clamped, 1f / GAMMA_EXP) - GAMMA_OFFSET;
    }

    public static Vector3D linearToSrgb(Vector3D v) {
        return new Vector3D(linearToSrgb(v.getX()), linearToSrgb(v.getY()), linearToSrgb(v.getZ()));
    }
}
