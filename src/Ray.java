public class Ray {
    private Vector3D pos;
    private Vector3D dir;

    Ray(Vector3D pos, Vector3D dir) {
        this.pos = pos;
        this.dir = dir.normalize();
    }

    public Vector3D getPos() {
        return pos;
    }

    public void setPos(Vector3D pos) {
        this.pos = pos;
    }

    public Vector3D getDir() {
        return dir;
    }

    public void setDir(Vector3D dir) {
        this.dir = dir.normalize();
    }

    public Vector3D at(float t) {
        // start + end*t
        return pos.add(dir.mul(t));
    }
}
