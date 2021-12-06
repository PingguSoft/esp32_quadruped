#include "Gait.h"

Vector Gait::_vecStep(30, 30, 20);
float  Gait::_stepsPerSec = 2.0f;

//: direction ratio calculation function
Vector Gait::calcMoveRatio(Vector move) {
    float maxV = max(abs(move.x), abs(move.y));

    return { move.x / maxV, move.y / maxV, move.z };
}

void Gait::setStepsPerSec(float steps) {
    for (int i = 0; i < BODY_NUM_LEGS; i++) {
        _paramLegs[i].reset();
    }
    _stepsPerSec = steps;
}

void Gait::setSwingOffsets(const float offsets[]) {
    for (int i = 0; i < BODY_NUM_LEGS; i++) {
        _paramLegs[i].setSwingOffset(offsets[i]);
    }
}

void Gait::doStep(int leg, Vector *move, Rotator *rot, bool enLog) {
    if (abs(move->x) == PRECISION && abs(move->y) == PRECISION) {
        _paramLegs[leg].reset();
        move->set(0, 0, move->z);
        return;
    }

    _paramLegs[leg].tick(*move, _vecStep, _fFreq, _stepsPerSec);
    Vector r = calcMoveRatio(*move);
    Vector c(r.x * _paramLegs[leg].getAmplitude(), r.y * _paramLegs[leg].getAmplitude(), move->z);

    if (_paramLegs[leg].isSwingState()) {
        float w0 = _vecStep.x * UNIT_MM * 4 * move->x;
        float l0 = _vecStep.y * UNIT_MM / 2 * move->y;
        float h0 = _vecStep.z * UNIT_MM;

        _iSwingLeg = leg;
        _fSwingAmplitude = sqrt(abs((1 - sq(c.x / w0) - sq(c.y / l0)) * sq(h0)));
        c.z = c.z - _fSwingAmplitude;
    } else {
        if (_isComp && _iSwingLeg >= 0) {
            float pct   = abs(_fSwingAmplitude / _vecStep.z);
            float roll  = (IS_RIGHT_LEG(_iSwingLeg) ? -pct : pct) * 2.0f;
            float pitch = (IS_FRONT_LEG(_iSwingLeg) ? -pct : pct) * 2.0f;

            rot->set(rot->yaw, pitch, roll);
        }
        c.set(-c.x, -c.y, c.z);
    }

    if (enLog) {
        LOG("tick:%3d, amplitude:%6.1f, swing:%d, (%6.1f, %6.1f, %6.1f)\n", _paramLegs[leg].getTick(), 
            _paramLegs[leg].getAmplitude(), int(_paramLegs[leg].isSwingState()), c.x, c.y, c.z);
    }

    move->set(c.x / UNIT_MM, c.y / UNIT_MM, c.z / UNIT_MM);
}
