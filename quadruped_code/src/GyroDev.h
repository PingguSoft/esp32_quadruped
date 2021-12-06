/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is derived from deviationTx project for Arduino.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 see <http://www.gnu.org/licenses/>
*/

#ifndef _GYRO_DEV_H_
#define _GYRO_DEV_H_
#include <Arduino.h>
#include <stdarg.h>
#include "config.h"
#include "I2Cdev.h"
#include "utils.h"

/*
*****************************************************************************************
* MACROS
*****************************************************************************************
*/
typedef struct {
    float psi;
    float theta;
    float phi;
} euler_t;

typedef struct {
    float yaw;
    float pitch;
    float roll;
} ypr_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} vec_t;

class GyroDev
{
public:
    void    setup(int16_t *offsets);
    uint8_t loop(void);
    bool    calibrate(int16_t *offsets);
    bool    isEnabled(void) { return _isEnable;  }
    euler_t getEuler(void)  { return _euler; }
    ypr_t   getYPR(void)    { return _ypr;   }
    vec_t   getGyro(void)   { return _gyro;  }
    vec_t   getAccel(void)  { return _accel; }

private:
    void    sleep(void);
    int8_t  cut(int val);
    float   delta(float val);
    
    //
    // MPU control/status vars
    bool          _isDmpReady   = false;      // set true if DMP init was successful
    uint8_t       _mpuIntStatus;              // holds actual interrupt status byte from MPU
    uint8_t       _devStatus;                 // return status after each device operation (0 = success, !0 = error)
    uint16_t      _packetSize;                // expected DMP packet size (default is 42 bytes)
    uint16_t      _fifoCount;                 // count of all bytes currently in FIFO
    uint8_t       _fifoBuffer[64];            // FIFO storage buffer
    //
    bool          _isEnable;
    ypr_t         _ypr;
    euler_t       _euler;
    vec_t         _gyro;
    vec_t         _accel;
};


#endif
