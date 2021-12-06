#include <Arduino.h>
#include "GyroDev.h"
#include "MPU6050_6Axis_MotionApps20.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/


/*
*****************************************************************************************
* MACROS
*****************************************************************************************
*/


/*
*****************************************************************************************
* TYPES
*****************************************************************************************
*/


/*
*****************************************************************************************
* CONSTS
*****************************************************************************************
*/


/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/
//
// below types can't be referenced in the header files due to library error
//
static MPU6050       mpu;    
// orientation/motion vars
static Quaternion    q;                         // [w, x, y, z]         quaternion container
static VectorFloat   gravity;                   // [x, y, z]            gravity vector

/*
*****************************************************************************************
* 
*****************************************************************************************
*/
int8_t GyroDev::cut(int val) {
    int8_t ret;

    ret = (val < -127) ? -127 : ((val > 127) ? 127 : val);
    return ret;
}

float GyroDev::delta(float val) {
    if (val > 180) {
        return val - 360;
    } else if (val < -180) {
        return val + 360;
    }
    return val;
}

/*
*****************************************************************************************
* 
*****************************************************************************************
*/
bool GyroDev::calibrate(int16_t *offsets) {
    _isDmpReady = false;

    mpu.initialize();
    _devStatus = mpu.dmpInitialize();

    mpu.setXAccelOffset(0);
    mpu.setYAccelOffset(0);
    mpu.setZAccelOffset(0);
    mpu.setXGyroOffset(0);
    mpu.setYGyroOffset(0);
    mpu.setZGyroOffset(0);

    if (_devStatus == 0) {
        // Calibration Time: generate offsets and calibrate our MPU6050
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);

        offsets[0] = mpu.getXAccelOffset();
        offsets[1] = mpu.getYAccelOffset();
        offsets[2] = mpu.getZAccelOffset();

        offsets[3] = mpu.getXGyroOffset();
        offsets[4] = mpu.getYGyroOffset();
        offsets[5] = mpu.getZGyroOffset();

        mpu.PrintActiveOffsets();

        _isDmpReady = true;
    }
    
    return _isDmpReady;
}

/*
*****************************************************************************************
* 
*****************************************************************************************
*/
void GyroDev::sleep(void) {
    mpu.setDMPEnabled(false);
    mpu.setSleepEnabled(false);
}


/*
*****************************************************************************************
* SETUP
*****************************************************************************************
*/
void GyroDev::setup(int16_t *offsets) {
    _isDmpReady = false;

    mpu.initialize();
    if (!mpu.testConnection()) {
        LOG("MPU6050 connection failed\n");
        return;
    }

    // load and configure the DMP
    LOG("Initializing DMP...\n");
    _devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    if (offsets != NULL) {
        mpu.setXAccelOffset(offsets[0]);
        mpu.setYAccelOffset(offsets[1]);
        mpu.setZAccelOffset(offsets[2]);
        mpu.setXGyroOffset(offsets[3]);
        mpu.setYGyroOffset(offsets[4]);
        mpu.setZGyroOffset(offsets[5]);
        //           X Accel  Y Accel  Z Accel   X Gyro   Y Gyro   Z Gyro
        //OFFSETS    -2962,    -121,    4536,     131,      30,     -11
    }

    if (_devStatus == 0) {
        // Calibration Time: generate offsets and calibrate our MPU6050
        // mpu.CalibrateAccel(6);
        // mpu.CalibrateGyro(6);
        // mpu.PrintActiveOffsets();
        
        // turn on the DMP, now that it's ready
        LOG("Enabling DMP...\n");
        mpu.setDMPEnabled(true);

        //attachInterrupt(digitalPinToInterrupt(PIN_IRQ), &GyroDev::dmpDataReady, RISING);
        _mpuIntStatus = mpu.getIntStatus();
        _isDmpReady = true;

        // get expected DMP packet size for later comparison
        _packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        LOG("DMP Initialization failed (code %d)\n", _devStatus);
    }

    _isEnable = true;
}

/*
*****************************************************************************************
* LOOP
*****************************************************************************************
*/
uint8_t GyroDev::loop(void) {
    uint8_t     ret = 0;
    float       buf[3];
    VectorInt16 result;
    VectorInt16 raw;

    if (!_isEnable || !_isDmpReady) {
        return ret;
    }

    _fifoCount = mpu.getFIFOCount();
    if (_fifoCount < _packetSize) {
        return ret;
    }

    _mpuIntStatus   = mpu.getIntStatus();        
    if ((_mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) || _fifoCount >= 1024) {
        mpu.resetFIFO();
        LOG("FIFO overflow!\n");
    } else if (_mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
        while (_fifoCount >= _packetSize) {
            mpu.getFIFOBytes(_fifoBuffer, _packetSize);
            _fifoCount -= _packetSize;
        }

        // Euler angles in degrees
        mpu.dmpGetQuaternion(&q, _fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);

#if 0
        mpu.dmpGetGyro((int16_t*)&_gyro, _fifoBuffer);
        mpu.dmpGetAccel(&raw, _fifoBuffer);
        mpu.dmpGetLinearAccel(&result, &raw, &gravity);
        //mpu.dmpGetLinearAccelInWorld(&result, &raw, &q);
        _accel.x = result.x;
        _accel.y = result.y;
        _accel.z = result.z;

        mpu.dmpGetEuler(buf, &q);
        _euler.psi   = buf[0] * 180 / M_PI;
        _euler.theta = buf[1] * 180 / M_PI;
        _euler.phi   = buf[2] * 180 / M_PI;
#endif
        //
        // mpu6050
        // y,-r,p
        //
        mpu.dmpGetYawPitchRoll(buf, &q, &gravity);
        _ypr.yaw   = degrees(buf[0]);
        _ypr.pitch = degrees(buf[2]);
        _ypr.roll  = degrees(buf[1]);
        
        ret = 1;
    }
    return ret;
}

