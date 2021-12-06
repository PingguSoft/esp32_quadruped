#include <FS.h>
#include <SPIFFS.h>
#include "GyroProc.h"

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


GyroProc::GyroProc(int hz) :
    _pidPit ("Pitch PID", true, 0.95, 0.35, 0),
    _pidRoll("Roll  PID", true, 0.95, 0.35, 0)
{
    _pidPit.setTarget(0.0f);
    _pidRoll.setTarget(0.0f);
    _iSel = 0;
    _iInterval = 1000 / hz;
    _pPID = &_pidPit;
    _p = _pPID->getP();
    _i = _pPID->getI();
    _d = _pPID->getD();
}

void GyroProc::setup() {
    memset(_offsets, 0, sizeof(_offsets));

    File file = SPIFFS.open(FILE_GYRO_CFG);
    if (file) {
        if (file.read((uint8_t*)_offsets, sizeof(_offsets)) > 0) {
            LOG("gyro cal data loaded !!!\n");
        }
        file.close();
        _gyro.setup(_offsets);
    } else {
        LOG("gyrocal.dat not found, start calibration !!\n");
        calibrate();
    }
}

bool GyroProc::calibrate(void) {
    bool ret;

    File file = SPIFFS.open(FILE_GYRO_CFG, FILE_WRITE);
    if (!file) {
        LOG("file write open error\n");
        return false;
    }
    ret = _gyro.calibrate(_offsets);
    file.write((uint8_t*)_offsets, sizeof(_offsets));
    file.close();
    _gyro.setup(_offsets);

    // LOG("cal offsets\n");
    // for (int i = 0; i < ARRAY_SIZE(_offsets); i++) {
    //     LOG("%6d, ", _offsets[i]);
    // }
    // LOG("\n");

    return ret;
}

float GyroProc::fixAngle(float fTgt, float fSrc) {
    if (abs(fTgt - fSrc) > 180) {
        if (fSrc > 0) {
            fTgt += 360;
        } else if (fSrc < 0) {
            fTgt -= 360;
        }
    }
    return fTgt;
}

bool GyroProc::isValid(float angle) {
    return !isnan(angle) && (-180 <= angle && angle <= 180);
}

void GyroProc::setTarget(float roll, float pit) {
    _pidRoll.setTarget(roll);
    _pidPit.setTarget(pit);
}

Rotator GyroProc::process(unsigned long ts, Rotator rot) {
    static Rotator ret;
    static int     ctr = 0;
    ypr_t   ypr;

    if (!_gyro.loop())
        return ret;

    ypr = _gyro.getYPR();
    if (!isValid(ypr.yaw) || !isValid(ypr.pitch) || !isValid(ypr.roll)) {
        return ret;
    }

    //  spotmicro gyro sensor output  kangal gyro sensor output       robot orientation
    //  yaw (left:-, right:+)         yaw (left:-, right:+)         yaw (left:-, right:+)
    //                                                                        HEAD
    //           pitch-                        pitch-                        pitch+
    //             |                             |                             |
    //    -roll ---+---  roll+          +roll ---+---  roll-          -roll ---+---  roll+
    //             |                             |                             |
    //           pitch+                        pitch+                        pitch-
    //

#if (CONFIG_BODY == CONFIG_BODY_SPOTMICRO)
    ypr.pitch = -ypr.pitch;
#elif (CONFIG_BODY == CONFIG_BODY_KANGAL)
    ypr.roll  = -ypr.roll;
    ypr.pitch = -ypr.pitch;
#endif

    ret.roll  = _pidRoll.compute(ts, ypr.roll, BODY_MAX_ROLL);
    ret.pitch = _pidPit.compute(ts, ypr.pitch, BODY_MAX_PITCH);
    ret.yaw   = rot.yaw;

    if (ctr == 0) {
        LOG("%10ld %6.2f, RP(%6.2f, %6.2f) OUT(%6.2f, %6.2f)\n", ts, ypr.yaw, ypr.roll, ypr.pitch,
            ret.roll, ret.pitch);
    }
    ctr = (ctr + 1) % 20;

    return ret;
}

void GyroProc::togglePID() {
    _iSel = (_iSel + 1) % 2;
    _pPID = (_iSel == 0) ? &_pidPit : &_pidRoll;
    _p = _pPID->getP();
    _i = _pPID->getI();
    _d = _pPID->getD();
    LOG("%s P:%6.2f, I:%6.2f, D:%6.2f\n", _pPID->getName().c_str(), _p, _i, _d);
}

void GyroProc::setPID() {
    _pPID->set(_p, _i, _d);
    LOG("%s P:%6.2f, I:%6.2f, D:%6.2f\n", _pPID->getName().c_str(), _pPID->getP(), _pPID->getI(), _pPID->getD());
}

void GyroProc::incP() {
    _p += 0.05f;
    setPID();
}

void GyroProc::decP() {
    _p -= 0.05f;
    setPID();
}

void GyroProc::incI() {
    _i += 0.05f;
    setPID();
}

void GyroProc::decI() {
    _i -= 0.05f;
    setPID();
}

void GyroProc::incD() {
    _d += 0.05f;
    setPID();
}

void GyroProc::decD() {
    _d -= 0.05f;
    setPID();
}

void GyroProc::reset() {
    _pidPit.reset();
    _pidRoll.reset();
}
