#ifndef _GAIT_H_
#define _GAIT_H_

#include "config.h"
#include "VecRot.h"
#include "hardware.h"
#include "utils.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
/*
    Z      TOP VIEW
               | -Y
               |
          ---------
          | 2     3 |               H
-X -------|    +    |-------- +X    E
          | 1     0 |               A
          ---------                 D
               |
               |
               | +Y
*/

/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/

/*
*****************************************************************************************
* GaitParam
*****************************************************************************************
*/
class GaitParam {
private:
    float    _fAmplitude;
    short    _tick;

    boolean  _isSwing;
    float    _fCurMult;
    float    _fSwingMult;
    float    _fStanceMult;
    float    _fSwingOffset;

public:
    GaitParam(float fSwingMult = 1.0f, float fStanceMult = 1.0f) {
        _tick = 0;
        _fSwingMult  = fSwingMult;
        _fStanceMult = fStanceMult;
        setSwingState(false);
    }

    void setSwingOffset(float fSwingOffset) {
        _fSwingOffset = fSwingOffset;

        if (fSwingOffset == 0) {
            setSwingState(true);
        } else {
            _fCurMult = fSwingOffset;
            _isSwing  = false;
        }
    }

    void setSwingState(boolean en) {
        _isSwing = en;
        if (en) {
            _fCurMult = _fSwingMult;
        } else {
            _fCurMult = _fStanceMult;
        }
    }

    void tick(Vector move, Vector step, float freq, float stepsPerSec) {
        float full = round(freq * _fCurMult / stepsPerSec);
        float w0   = step.x * UNIT_MM / (2 / max(abs(move.x), abs(move.y)));
        float a0   = (w0 * 2) * (float(_tick) / full) - w0;

        _tick++;
        _fAmplitude = a0;
        if (_tick > full) {
            setSwingState(!_isSwing);
            _fAmplitude = -w0;
            _tick = 1;
        }
    }

    void reset() {
        _tick = 0;
        setSwingOffset(_fSwingOffset);
    }

    void setMult(float fSwingMult = 1.0f, float fStanceMult = 1.0f) {
        _fSwingMult  = fSwingMult;
        _fStanceMult = fStanceMult;
    }

    boolean isSwingState()              { return _isSwing;  }
    float   getAmplitude()              { return _fAmplitude;  }
    int     getTick()                   { return _tick; }
};

/*
*****************************************************************************************
* Gait
*****************************************************************************************
*/
class Gait {
public:
    Gait(float fSwingMult = 1.0f, float fStanceMult = 1.0f, float freq = HW_SERVO_UPDATE_FREQ) {
        _fFreq     = freq;
        _isComp    = false;
        _iSwingLeg = -1;

        for (int i = 0; i < BODY_NUM_LEGS; i++) {
            _paramLegs[i].setMult(fSwingMult, fStanceMult);
        }
    }

    virtual ~Gait() {}

    void    setComp(boolean en)                 { _isComp  = en;              }
    void    setStep(Vector vecStep)             { _vecStep = vecStep;         }
    Vector *getStep(void)                       { return &_vecStep;           }
    float   getStepsPerSec(void)                { return _stepsPerSec;        }
    void    setStepsPerSec(float steps);

    virtual String getName(void) = 0;
    virtual void   doStep(int leg, Vector *move, Rotator *rot, bool enLog);

protected:
    Vector calcMoveRatio(Vector move);
    void   setSwingOffsets(const float offsets[]);

    float           _fFreq;
    GaitParam       _paramLegs[BODY_NUM_LEGS];
    float           _fSwingAmplitude;
    int             _iSwingLeg;
    bool            _isComp;

    static Vector   _vecStep;
    static float    _stepsPerSec;

};


/*
*****************************************************************************************
* GaitTrot
*****************************************************************************************
*/
class GaitTrot : public Gait {
private:
    const float _offsets[BODY_NUM_LEGS] = {  0, 2, 0, 2 };          // RF, RH, LH, LF

public:
    GaitTrot(float freq) : Gait(2.0f, 2.0f, freq) {
        setSwingOffsets(_offsets);
    }
    virtual ~GaitTrot() {}

    String getName(void) { return "Trot"; }
};

#if 0
/*
*****************************************************************************************
* GaitCrawl
*****************************************************************************************
*/
class GaitCrawl : public Gait {
private:
    const float _offsets[BODY_NUM_LEGS] = { 0, 3.0f, 1.0f, 2.0f };  // RF, RH, LH, LF

public:
    GaitCrawl(float freq) : Gait(1.0f, 3.0f, freq) {
        setSwingOffsets(_offsets);
        setComp(true);
    }

    String getName(void) { return "Crawl"; }
};
#endif

/*
*****************************************************************************************
* GaitLateral
*****************************************************************************************
*/
class GaitLateral : public Gait {
private:
    const float _offsets[BODY_NUM_LEGS] = { 2.0f, 0.5f, 2.5f, 0.0f };  // RF, RH, LH, LF

public:
    GaitLateral(float freq) : Gait(1.0f, 3.0f, freq) {
        setSwingOffsets(_offsets);
        setComp(true);
    }

    String getName(void) { return "Lateral"; }
};

/*
*****************************************************************************************
* GaitDiagonal
*****************************************************************************************
*/
class GaitDiagonal : public Gait {
private:
    const float _offsets[BODY_NUM_LEGS] = { 1.8f, 3.0f, 1.2f, 0.0f };  // RF, RH, LH, LF

public:
    GaitDiagonal(float freq) : Gait(1.0f, 3.0f, freq) {
        setSwingOffsets(_offsets);
        setComp(true);
    }

    String getName(void) { return "Diagonal"; }
};

/*
*****************************************************************************************
* CLASS
*****************************************************************************************
*/
#define GAIT_CTR    3

class GaitMan {
private:

public:
    GaitMan() {
        _pGaits[0] = new GaitTrot(HW_SERVO_UPDATE_FREQ);
        _pGaits[1] = new GaitLateral(HW_SERVO_UPDATE_FREQ);
        _pGaits[2] = new GaitDiagonal(HW_SERVO_UPDATE_FREQ);
    }

    Gait *get(int idx) {
        if (idx < GAIT_CTR)
            return _pGaits[idx];
        return NULL;
    }

    int getGaitCnt() { return GAIT_CTR; }

private:
    Gait    *_pGaits[GAIT_CTR];
};

#endif
