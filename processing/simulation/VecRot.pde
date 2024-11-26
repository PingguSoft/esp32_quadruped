

public class Vector {
    public float x;
    public float y;
    public float z;

    public Vector() {
        set(0, 0, 0);
    }

    public Vector(float x, float y) {
        set(x, y, 0);
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

    public void zero() {
        set(0, 0, 0);
    }

    public boolean equals(Vector v) {
        return (x == v.x && y == v.y && z == v.z);
    }

    void dump(String name) {
        print(String.format("[%4s] x:%7.3f, y:%7.3f, z:%7.3f\n", name, x, y, z));
    }
};

public class Rotator {
    public float yaw;
    public float pitch;
    public float roll;

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

    public void zero() {
        set(0, 0, 0);
    }

    public boolean equals(Rotator r) {
        return (yaw == r.yaw && pitch == r.pitch && roll == r.roll);
    }

    void dump(String name) {
        print(String.format("[%4s] y:%7.3f, p:%7.3f, r:%7.3f\n", name, yaw, pitch, roll));
    }
};
