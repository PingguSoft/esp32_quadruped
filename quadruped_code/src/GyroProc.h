#ifndef _GYRO_PROC_H_
#define _GYRO_PROC_H_

#include "config.h"
#include "VecRot.h"
#include "GyroDev.h"
#include "PIDController.h"
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

class GyroProc {
public:
    GyroProc(int hz);

    void    setup();
    bool    calibrate(void);
    bool    isValid(float angle);
    Rotator process(unsigned long ts, Rotator rot);
    void    togglePID();
    void    incP();
    void    decP();
    void    incI();
    void    decI();
    void    incD();
    void    decD();
    void    reset();
    void    setTarget(float roll, float pit);

private:
    void  setPID();
    float fixAngle(float fTgt, float fSrc);

    GyroDev         _gyro;

    PIDController   _pidPit;
    PIDController   _pidRoll;
    PIDController  *_pPID;

    Rotator         _rotTgt;

    int             _iInterval;
    float           _p, _i, _d;
    int             _iSel;
    int16_t         _offsets[6];
};

#endif