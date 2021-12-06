#include "PIDController.h"


/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
PIDController::PIDController(String name, bool circular, float p, float i, float d) {
    _strName = name;
    set(p, i, d);
    _fTarget = 0;
    _fIntegralLimit = 0;
    _isCircular = circular;
}

PIDController::PIDController(String name, float p, float i, float d) : PIDController(name, false, p, i, d) {
}

PIDController::PIDController(String name, float p, float i, float d, float iLimit) : PIDController(name, false, p, i, d) {
    _fIntegralLimit = iLimit;
}

PIDController::PIDController(String name, bool circular, float p, float i, float d, float iLimit) : PIDController(name, circular, p, i, d) {
    _fIntegralLimit = iLimit;
}

void PIDController::reset() {
    _fIntegral            = 0;
    _lLastTS              = 0;
    _fLastInput           = 0;
    _fIntegStableCheck    = 0;
    _fDurationStableCheck = 0;
    _isStableCheck = false;
}

String PIDController::getName() {
    return _strName;
}

void PIDController::set(float p, float i, float d) {
    _p = p;
    _i = i;
    _d = d;
    reset();
}

float PIDController::getP() {
    return _p;
}

float PIDController::getI() {
    return _i;
}

float PIDController::getD() {
    return _d;
}

void PIDController::setTarget(float target) {
    _fTarget = target;
}

float PIDController::getTarget() {
    return _fTarget;
}

float PIDController::getLastInput() {
    return _fLastInput;
}

float PIDController::calcError(float a, float b) {
    float error;

    error = a - b;
    if (_isCircular) {
        if (error > 180)
            error = error -360;
        else if (error < -180)
            error = error + 360;
    }
    return error;
}

float PIDController::computeWithDelta(float input, float delta) {
    float error;
    float dError;
    float output;
    float outP;
    float outI;
    float outD;

    error = calcError(_fTarget, input);
    dError = calcError(error, _fLastError);

    if (_isStableCheck) {
        _fDurationStableCheck += delta;
        _fIntegStableCheck += (error * delta);
    }

    _fIntegral += (error * delta);
    if (_fIntegralLimit != 0) {
        _fIntegral = fConstrain(_fIntegral, -_fIntegralLimit, _fIntegralLimit, 0);
    }

    outP    = _p * error;
    outI    = _i * _fIntegral;
    outD    = _d * (dError / delta);
    output  = outP + outI + outD;
    //LOG("t:%5.2f, i:%5.2f, e:%5.2f, d:%5.2f, P:%5.2f, i:%5.2f, oD:%5.2f, out:%5.2f\n", _fTarget, input, error, delta, outP, _fIntegral, outD, output);

    _fLastInput = input;
    _fLastError = error;
    // clamp integral sum if the sign is changed
    //        if ((dError < 0 && output > 0) || (dError > 0 || output < 0)) {
    //            _fIntegral = 0;
    //        }
    return output;
}

float PIDController::compute(unsigned long now, float input) {
    float  delta = (_lLastTS == 0 || (now == _lLastTS)) ? 0.02f : ((now - _lLastTS) / 1000.0f);

    _lLastTS = now;
    return computeWithDelta(input, delta);
}

float PIDController::compute(unsigned long now, float input, float limit) {
    float out = compute(now, input);
    return fConstrain(out, -limit, limit, 0.0);
}

float PIDController::computeWithDelta(float input, float limit, float delta) {
    float out = computeWithDelta(input, delta);
    return fConstrain(out, -limit, limit, 0.0);
}

bool PIDController::isStable(float duration, float v) {
    bool ret = false;

    if (!_isStableCheck) {
        _isStableCheck = true;
        return ret;
    }

    if (_fDurationStableCheck > duration) {
        float avg = abs(_fIntegStableCheck / _fDurationStableCheck);
        //LOG("duration:%f, integ:%f, avg:%f", _fDurationStableCheck, _fIntegStableCheck, avg);

        if (avg > v) {
            // reset stable check window
            _fDurationStableCheck = 0;
            _fIntegStableCheck = 0;
        } else {
            ret = true;
        }
    }
    return ret;
}

float PIDController::fConstrain(float v, float min, float max, float deadzone) {
    if (v < min)
        v = min;
    else if (v > max)
        v = max;

    if (-deadzone < v && v < deadzone)
        v = 0;

    return v;
}    
