#ifndef _QUAD_RUPED_H
#define _QUAD_RUPED_H

#include "config.h"
#include "VecRot.h"
#include "hardware.h"
#include "Gait.h"
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
class JointAngle {
public:
    JointAngle() {
        _coxa  = 0.0f;
        _femur = 0.0f;
        _tibia = 0.0f;
    }

    void set(float coxa, float femur, float tibia) {
        _coxa  = coxa;
        _femur = femur;
        _tibia = tibia;
    }

    float getCoxa()  { return _coxa;  }
    float getFemur() { return _femur; }
    float getTibia() { return _tibia; }

private:
    float _coxa;
    float _femur;
    float _tibia;
};

/*
*****************************************************************************************
* CLASS
*****************************************************************************************
*/
class Leg {
public:
    Leg() {
        _isDebug    = false;
    };

    void set(int pos, float bodyFrontWidth, float bodyHeight, float bodyMiddleWidth,
        float coxaLength, float femurLength, float tibiaLength);

    void  bodyIK(Vector *pMov, Rotator *pRot, Vector *pTgt);
    void  legIK(Vector *pTgt, JointAngle *pJA);
    void  move(Vector *pMov, Rotator *pRot);
    void  enableDebug(bool en)        { _isDebug = en;       }
    JointAngle *getJointPtr()         { return &_jointAngle; }
    static Vector *getTipOffset(void) { return &_vTipOffset; }

private:
    float fixAngle(float angle);
    float roundUp(float v);

    int   _pos;
    bool  _isDebug;
    float _coxaLength;
    float _femurLength;
    float _tibiaLength;
    float _coxaOffsetZ;

    Vector      _vOffFromCenter;
    Vector      _vInitPos;
    JointAngle  _jointAngle;

    static Vector _vTipOffset;
};

class QuadRuped {
public:
    QuadRuped(float bodyWidth, float bodyHeight, float coxaLen, float coxaOffsetZ, float femurLen, float tibiaLen);

    void    setup(Hardware *pHW);
    Vector *getTipOffset()      { return Leg::getTipOffset(); }
    void    setGait(Gait *gait) { _pGait = gait; }
    Gait   *getGait()           { return _pGait; }
    int     getDebugMask()      { return _debugLegMask;       }

    void setDebugMask(int mask);
    void update(unsigned long ts, boolean isWalk, Vector *pMov, Rotator *pRot);

private:
    Leg       _legs[BODY_NUM_LEGS];
    Vector   _vBodyPos;
    Gait     *_pGait;
    Hardware *_pHW;
    int       _iInterval;         // (unit:ms)
    unsigned long _lLastChcked;
    int       _debugLegMask;
};

#endif