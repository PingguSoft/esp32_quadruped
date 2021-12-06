#ifndef _VECROT_H_
#define _VECROT_H_

#include "config.h"
#include "utils.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/

/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/


/*
*****************************************************************************************
* CLASS
*****************************************************************************************
*/
class Vector {
public:
    float x;
    float y;
    float z;

    Vector() {
        set(0, 0, 0);
    }

    Vector(float x, float y, float z) {
        set(x, y, z);
    }

    void set(Vector v) {
        set(v.x, v.y, v.z);
    }

    void set(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    void zero() {
        set(0, 0, 0);
    }

    bool equals(Vector v) {
        return (x == v.x && y == v.y && z == v.z);
    }

    void dump(String name) {
        LOG("[%4s] x:%7.3f, y:%7.3f, z:%7.3f\n", name.c_str(), x, y, z);
    }
};

class Rotator {
public:
    float yaw;
    float pitch;
    float roll;

    Rotator() {
        set(0, 0, 0);
    }

    Rotator(Rotator *r) {
        set(r->yaw, r->pitch, r->roll);
    }

    Rotator(float yaw, float pitch, float roll) {
        set(yaw, pitch, roll);
    }

    void set(Rotator r) {
        set(r.yaw, r.pitch, r.roll);
    }

    void set(float yaw, float pitch, float roll) {
        this->yaw = yaw;
        this->pitch = pitch;
        this->roll = roll;
    }

    void zero() {
        set(0, 0, 0);
    }

    bool equals(Rotator r) {
        return (yaw == r.yaw && pitch == r.pitch && roll == r.roll);
    }

    void dump(String name) {
        LOG("[%4s] y:%7.3f, p:%7.3f, r:%7.3f\n", name.c_str(), yaw, pitch, roll);
    }
};

#endif