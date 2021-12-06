#ifndef _TIMED_MOVE_H_
#define _TIMED_MOVE_H_

#include "config.h"
#include "VecRot.h"

class TimedMove {
private:
    Vector          _vecDirInc;
    Rotator         _rotDirInc;
    Vector          _vecDirTgt;
    Rotator         _rotDirTgt;
    float           _msToGo;
    unsigned long   _lLastTS;
    Vector          *_pDestVec;
    Rotator         *_pDestRot;

public:
    TimedMove() {
    }

    void setup(Vector *pVec, Rotator *pRot) {
        _pDestVec = pVec;
        _pDestRot = pRot;
    }

    void go(Vector vecTgt, Rotator rotTgt, float ms) {
        float x = (vecTgt.x - _pDestVec->x) / ms;
        float y = (vecTgt.y - _pDestVec->y) / ms;
        float z = (vecTgt.z - _pDestVec->z) / ms;
        _vecDirInc.set(x, y, z);
        _vecDirTgt.set(vecTgt);
        vecTgt.dump("target");
        _vecDirInc.dump("dir_inc");

        x = (rotTgt.yaw - _pDestRot->yaw) / ms;
        y = (rotTgt.pitch - _pDestRot->pitch) / ms;
        z = (rotTgt.roll - _pDestRot->roll) / ms;
        _rotDirInc.set(x, y, z);
        _rotDirTgt.set(rotTgt);
        _msToGo  = ms;
        _lLastTS = millis();
    }

    void handle(unsigned long ts) {
        int diff = int(ts - _lLastTS);

        if (diff > 100) {
            diff = 20;
        }

        // every 20ms
        if (diff >= 20) {
            _lLastTS = ts;

            if (_msToGo > 0) {
                _msToGo -= diff;
                if (_msToGo <= 0) {
                    *_pDestVec = _vecDirTgt;
                    *_pDestRot = _rotDirTgt;
                } else {
                    _pDestVec->x += (_vecDirInc.x * diff);
                    _pDestVec->y += (_vecDirInc.y * diff);
                    _pDestVec->z += (_vecDirInc.z * diff);

                    _pDestRot->yaw   += (_rotDirInc.yaw * diff);
                    _pDestRot->pitch += (_rotDirInc.pitch * diff);
                    _pDestRot->roll  += (_rotDirInc.roll * diff);
                }
            }
        }
    }
};
#endif