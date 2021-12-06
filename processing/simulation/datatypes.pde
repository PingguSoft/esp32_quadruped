final class Vector {
    public float x, y, z;

    public Vector() {
        set(0, 0, 0);
    }

    public Vector(float x, float y, float z) {
        set(x, y, z);
    }

    public void set(Vector v) {
        set(v.x, v.y, v.z);
    }

    public void set(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    void zero() {
        set(0, 0, 0);
    }

    public float x() {
        return x;
    }

    public float y() {
        return y;
    }

    public float z() {
        return z;
    }
    
    public void dump(String name) {
        println(String.format("[%4s] x:%7.3f, y:%7.3f, z:%7.3f", name, x, y, z));
    }
}


final class Rotator {
    public float yaw, pitch, roll;

    public Rotator() {
        set(0, 0, 0);
    }
    
    public Rotator(Rotator r) {
        set(r.yaw, r.pitch, r.roll);
    }

    public Rotator(float yaw, float pitch, float roll) {
        set(yaw, pitch, roll);
    }

    public void set(Rotator r) {
        set(r.yaw, r.pitch, r.roll);
    }

    public void set(float yaw, float pitch, float roll) {
        this.yaw = yaw;
        this.pitch = pitch;
        this.roll = roll;
    }

    void zero() {
        set(0, 0, 0);
    }

    public float yaw() {
        return yaw;
    }

    public float pitch() {
        return pitch;
    }

    public float roll() {
        return roll;
    }

    public void dump(String name) {
        println(String.format("[%4s] y:%7.3f, p:%7.3f, r:%7.3f", name, yaw, pitch, roll));
    }
}
