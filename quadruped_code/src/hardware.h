#ifndef _HARDWARE_H_
#define _HARDWARE_H_

//#define  ENABLE_DEBUG_OUTPUT
#include <Adafruit_PWMServoDriver.h>
#include "config.h"
#include "VecRot.h"
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
class Hardware {
    struct _joint {
        int min;        // min    angle
        int max;        // max    angle
        int offset;     // offset angle
    };

    struct _servo_cfg {
        uint8_t pin;    // pin
        int8_t  sign;   // direction sign (1:normal, -1:inverted)
        int     angle;  // last angle
        int     uS;     // micro second
    };

public:
    Hardware();
    void setup(void);
    void setJoint(int leg, int joint, int deg10);
    void setLeg(int leg, int a10, int b10, int c10);
    void calibrateLegs(int key);
    void dump();
    int  checkBattery();
    int  getServoFreq()             { return HW_SERVO_UPDATE_FREQ; }
#if CONFIG_ENABLE_CAM_PAN_TILT
    void setPanTilt(int pan, int tilt);
#endif

private:
    void loadConfig(void);
    void saveConfig(void);
    uint16_t analogReadAvg(uint16_t pin);
#if CONFIG_ENABLE_CAM_PAN_TILT
    void setPanTiltInternal(int ch, int deg10);
#endif

    int                             _calAngles[BODY_NUM_LEGS][BODY_NUM_JOINTS];
    Adafruit_PWMServoDriver         _servo;

    static const char              *_kLegNames[BODY_NUM_LEGS];
    static const struct _joint      _kJointRanges[BODY_NUM_JOINTS];
    static const float              _kParentDir[BODY_NUM_LEGS][BODY_NUM_JOINTS - 1];

    static struct _servo_cfg        _cfgServos[BODY_NUM_LEGS][BODY_NUM_JOINTS];

#if CONFIG_ENABLE_CAM_PAN_TILT
    static struct _servo_cfg        _cfgPanTiltServos[2];
    int                             _calAnglesPanTilt[2];
#endif
};

#endif