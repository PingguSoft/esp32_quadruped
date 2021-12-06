#ifndef _PID_CONTROLLER_H_
#define _PID_CONTROLLER_H_

#include <Arduino.h>

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/

class PIDController {
private:
    String  _strName;
    float   _p;
    float   _i;
    float   _d;
    unsigned long _lLastTS;
    float   _fTarget;
    float   _fLastInput;
    float   _fLastError;
    float   _fIntegral;
    float   _fIntegralLimit;
    bool    _isCircular;
    bool    _isStableCheck;
    float   _fIntegStableCheck;
    float   _fDurationStableCheck;

public:
    PIDController(String name, bool circular, float p, float i, float d);
    PIDController(String name, float p, float i, float d);
    PIDController(String name, float p, float i, float d, float iLimit);
    PIDController(String name, bool circular, float p, float i, float d, float iLimit);

    void  reset();
    String getName();
    void  set(float p, float i, float d);
    float getP();
    float getI();
    float getD();
    void  setTarget(float target);
    float getTarget();
    float compute(unsigned long now, float input);
    float compute(unsigned long now, float input, float limit);
    bool  isStable(float duration, float v);
private:
    float getLastInput();
    float calcError(float a, float b);
    float computeWithDelta(float input, float delta);
    float computeWithDelta(float input, float limit, float delta);
    float fConstrain(float v, float min, float max, float deadzone);
};

#endif
