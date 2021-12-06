#include <Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include "hardware.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
#if (CONFIG_BODY == CONFIG_BODY_SPOTMICRO)
    static const int kSERVO_PERIOD_MIN   =  650;
    static const int kSERVO_PERIOD_MAX   = 2350;
#elif (CONFIG_BODY == CONFIG_BODY_KANGAL)
    //SPT5430HV-180W
    //max    -90    0    90  min
    //2300  2200  1370  520  380
    static const int kSERVO_PERIOD_MIN   =  520;
    static const int kSERVO_PERIOD_MAX   = 2220;
#endif

static const int kDEG10_MIN          = 0;
static const int kDEG10_MAX          = 1800;

/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/
/*
    Z      TOP VIEW
    |
    |     ---------
    |     | 2     3 |
    |     |    +    | head
    |     | 1     0 |
    |     ---------
    |
----+---------------------> X
    |
    |
*/


/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/
const struct Hardware::_joint Hardware::_kJointRanges[BODY_NUM_JOINTS] = {
    // min, max, default offset
    {-900,  900,  900},     // coxa
    {-900,  900,  900},     // femur
    {-900,  900,  450}      // tibia  (angle - 45)
};

// top view
#if (CONFIG_BODY == CONFIG_BODY_SPOTMICRO)
    struct Hardware::_servo_cfg Hardware::_cfgServos[BODY_NUM_LEGS][BODY_NUM_JOINTS] = {
        //   coxa          femur         tibia            leg no.
        { { 12,  1, 0 }, { 13,  1, 0 }, { 14, -1, 0 } },  // right front
        { {  8, -1, 0 }, {  9,  1, 0 }, { 10, -1, 0 } },  // right rear
        { {  4, -1, 0 }, {  5, -1, 0 }, {  6,  1, 0 } },  // left  rear
        { {  0,  1, 0 }, {  1, -1, 0 }, {  2,  1, 0 } }   // left  front
    };
#elif (CONFIG_BODY == CONFIG_BODY_KANGAL)
    // Please note that: The movement of the femur is effects to the tibia.
    // So it works dependent on it. Shortly; new tibia angle = tibia angle + femur angle
    struct Hardware::_servo_cfg Hardware::_cfgServos[BODY_NUM_LEGS][BODY_NUM_JOINTS] = {
        //   coxa          femur         tibia            leg no.
        { {  2,  1, 0 }, {  1,  1, 0 }, {  0, -1, 0 } },  // right front
        { { 12, -1, 0 }, { 14,  1, 0 }, { 13, -1, 0 } },  // right rear
        { {  8, -1, 0 }, { 10, -1, 0 }, {  9,  1, 0 } },  // left  rear
        { {  6,  1, 0 }, {  5, -1, 0 }, {  4,  1, 0 } }   // left  front
    };
#endif

#if CONFIG_ENABLE_CAM_PAN_TILT
struct Hardware::_servo_cfg Hardware::_cfgPanTiltServos[2] = {
    {  7, 1, 0}, {  3, 1, 0},
//    { 11, 1, 0}, { 15, 1, 0},
};
#endif

/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
Hardware::Hardware() {
    memset(&_calAngles, 0, sizeof(_calAngles));
    memset(&_calAnglesPanTilt, 0, sizeof(_calAnglesPanTilt));
}

void Hardware::setup(void) {
    _servo = Adafruit_PWMServoDriver(PCA9685_I2C_ADDRESS, Wire);
    loadConfig();

    _servo.begin();
    _servo.setOscillatorFrequency(27000000);
    _servo.setPWMFreq(getServoFreq());
    Wire.setClock(400000);

    setPanTilt(900, 900);
}

void Hardware::loadConfig(void) {
    File file = SPIFFS.open(FILE_SERVO_CFG);
    if (!file) {
        LOG("file not found\n");
        return;
    }
    file.read((uint8_t*)&_calAngles, sizeof(_calAngles));
#if CONFIG_ENABLE_CAM_PAN_TILT
    file.read((uint8_t*)&_calAnglesPanTilt, sizeof(_calAnglesPanTilt));
#endif
    file.close();
}

void Hardware::saveConfig(void) {
    File file = SPIFFS.open(FILE_SERVO_CFG, FILE_WRITE);
    if (!file) {
        LOG("file not found\n");
        return;
    }
    file.write((uint8_t*)&_calAngles, sizeof(_calAngles));
#if CONFIG_ENABLE_CAM_PAN_TILT
    file.write((uint8_t*)&_calAnglesPanTilt, sizeof(_calAnglesPanTilt));
#endif
    file.close();
}

/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
void Hardware::dump() {
    LOG("---------- leg dump ----------\n");
    for (int i = 0; i < BODY_NUM_LEGS; i++) {
        LOG("leg:%d ", i + 1);
        for (int j = 0; j < 3; j++) {
            LOG("(%5.1f, %5d, %4d), ", _cfgServos[i][j].angle / 10.0f, _cfgServos[i][j].uS, _calAngles[i][j]);
        }
        LOG("\n");
    }

#if CONFIG_ENABLE_CAM_PAN_TILT
    for (int j = 0; j < 2; j++) {
        LOG("(%5.1f, %5d, %4d), ", _cfgPanTiltServos[j].angle / 10.0f, _cfgPanTiltServos[j].uS, _calAnglesPanTilt[j]);
    }
    LOG("\n");
#endif

    LOG("------------------------------\n");
}

#if CONFIG_ENABLE_CAM_PAN_TILT
void Hardware::setPanTiltInternal(int ch, int deg10) {
    int uS;
    int adjDeg;

    adjDeg = constrain(deg10, kDEG10_MIN, kDEG10_MAX);
    adjDeg = adjDeg + _calAnglesPanTilt[ch];
    uS = (_cfgPanTiltServos[ch].sign == 1) ?
            map(adjDeg, kDEG10_MIN, kDEG10_MAX, kSERVO_PERIOD_MIN, kSERVO_PERIOD_MAX) :
            map(adjDeg, kDEG10_MIN, kDEG10_MAX, kSERVO_PERIOD_MAX, kSERVO_PERIOD_MIN);

    _cfgPanTiltServos[ch].angle = deg10;
    _cfgPanTiltServos[ch].uS    = uS;
    _servo.writeMicroseconds(_cfgPanTiltServos[ch].pin, uS);
}

void Hardware::setPanTilt(int pan, int tilt) {
    setPanTiltInternal(0, pan);
    setPanTiltInternal(1, tilt);
}
#endif

void Hardware::setJoint(int leg, int joint, int deg10) {
    int uS;
    int adjDeg;

    adjDeg = constrain(deg10, _kJointRanges[joint].min, _kJointRanges[joint].max);
    adjDeg = adjDeg + _kJointRanges[joint].offset + _calAngles[leg][joint];
    adjDeg = constrain(adjDeg, kDEG10_MIN, kDEG10_MAX);

    uS = (_cfgServos[leg][joint].sign == 1) ?
            map(adjDeg, kDEG10_MIN, kDEG10_MAX, kSERVO_PERIOD_MIN, kSERVO_PERIOD_MAX) :
            map(adjDeg, kDEG10_MIN, kDEG10_MAX, kSERVO_PERIOD_MAX, kSERVO_PERIOD_MIN);

    //LOG("leg:%d, joint:%d, sign:%2d, angle:%5.1f => %5.1f, cal:%5.1f, uS:%5d\n", leg + 1, joint, _cfgServos[leg][joint].sign, deg10 / 10.0f, adjDeg / 10.0f, _calAngles[leg][joint] / 10.0f, uS);

    _cfgServos[leg][joint].angle = deg10;
    _cfgServos[leg][joint].uS    = uS;

    _servo.writeMicroseconds(_cfgServos[leg][joint].pin, uS);
}

void Hardware::setLeg(int leg, int a10, int b10, int c10) {
    setJoint(leg, 0, a10);
    setJoint(leg, 1, b10);
    setJoint(leg, 2, c10);
}

void Hardware::calibrateLegs(int key) {
    static int leg   = 0;
    static int joint = 0;

    switch (key) {
        case 'q':
            LOG("IDLE POSE (ALL 0 degree) !\n");
            for (int i = 0; i < BODY_NUM_LEGS; i++)
                setLeg(i, 0, 0, 0);
            break;

        case 'w':
            LOG("READY POSE !\n");
            for (int i = 0; i < BODY_NUM_LEGS; i++)
                setLeg(i, 0, -450, 0);
            return;

#if (CONFIG_BODY == CONFIG_BODY_SPOTMICRO)
        case 'e':
            LOG("STRAIGHT POSE !\n");
            for (int i = 0; i < BODY_NUM_LEGS; i++)
                setLeg(i, 0, 0, 900);
            return;
#endif
        case 'c':
            LOG("calibration cleared !\n");
            memset(&_calAngles, 0, sizeof(_calAngles));
            return;

        case 's':
            saveConfig();
            LOG("calibration saved !\n");
            return;

        case 'l':
            loadConfig();
            LOG("calibration loaded !\n");

        case 'p':
            LOG("---------- cal ----------\n");
            for (int i = 0; i < BODY_NUM_LEGS; i++) {
                LOG("leg:%d, %4d, %4d, %4d\n", i + 1, _calAngles[i][0], _calAngles[i][1], _calAngles[i][2]);
            }
#if CONFIG_ENABLE_CAM_PAN_TILT
            LOG("p&t:5, %4d, %4d\n", _calAnglesPanTilt[0], _calAnglesPanTilt[1]);
#endif
            LOG("-------------------------\n");
            return;

        // leg selection
        case '1':
        case '2':
        case '3':
        case '4':
            leg   = key - '1';
            joint = 0;
            break;

#if CONFIG_ENABLE_CAM_PAN_TILT
        case '5':
            leg   = key - '1';
            joint = 0;
            break;
#endif

        // joint selection
        case '7':
        case '8':
        case '9':
            joint = key - '7';
            break;

        // cal +/-
        case '=':
        case '+':
            if (leg < BODY_NUM_LEGS) {
                _calAngles[leg][joint] += 5;   // 0.5 degree
            } else {
#if CONFIG_ENABLE_CAM_PAN_TILT
                _calAnglesPanTilt[joint] += 5;
#endif
            }
            break;

        case '-':
            if (leg < BODY_NUM_LEGS) {
                _calAngles[leg][joint] -= 5;   // 0.5 degree
            } else {
#if CONFIG_ENABLE_CAM_PAN_TILT
                if (joint < 2)
                    _calAnglesPanTilt[joint] -= 5;
#endif
            }
            break;

        // angle +/- for test
        case ']':
            if (leg < BODY_NUM_LEGS) {
                _cfgServos[leg][joint].angle += 5;
            } else {
#if CONFIG_ENABLE_CAM_PAN_TILT
                if (joint < 2)
                    _cfgPanTiltServos[joint].angle += 5;
#endif
            }
            break;

        case '[':
            if (leg < BODY_NUM_LEGS) {
                _cfgServos[leg][joint].angle -= 5;
            } else {
#if CONFIG_ENABLE_CAM_PAN_TILT
                if (joint < 2)
                    _cfgPanTiltServos[joint].angle -= 5;
#endif
            }
            break;
    }

    if (key > 0) {
        if (0 <= leg && leg < BODY_NUM_LEGS && joint >= 0) {
            setJoint(leg, joint, _cfgServos[leg][joint].angle);
            LOG("leg:%d, joint:%d, sign:%2d, angle:%5.1f, cal:%5.1f, uS:%5d\n", leg + 1, joint, _cfgServos[leg][joint].sign,
                _cfgServos[leg][joint].angle / 10.0f, _calAngles[leg][joint] / 10.0f, _cfgServos[leg][joint].uS);
        }

#if CONFIG_ENABLE_CAM_PAN_TILT
        if (leg == BODY_NUM_LEGS && joint < 2) {
            setPanTiltInternal(joint, _cfgPanTiltServos[joint].angle);
            LOG("leg:%d, joint:%d, sign:%2d, angle:%5.1f, cal:%5.1f, uS:%5d\n", leg + 1, joint, _cfgPanTiltServos[joint].sign,
                _cfgPanTiltServos[joint].angle / 10.0f, _calAnglesPanTilt[joint] / 10.0f, _cfgPanTiltServos[joint].uS);
        }
#endif
    }
}

#define ANALOG_RETRY_COUNT      5

uint16_t Hardware::analogReadAvg(uint16_t pin) {
    uint16_t v;
    uint32_t sum = 0;
    uint16_t values[ANALOG_RETRY_COUNT];

    for (uint8_t i = 0; i < ANALOG_RETRY_COUNT; i++) {
        v = analogRead(pin);
        values[i] = v;
        sum += v;
    }

    uint16_t avg = sum / ANALOG_RETRY_COUNT;
    uint8_t  cnt = ANALOG_RETRY_COUNT;
    for (uint8_t i = 0; i < ANALOG_RETRY_COUNT; i++) {
        v = values[i];
        if (abs(v - avg) > 100) {
            sum -= v;
            cnt --;
        }
    }
    return (cnt > 0) ? (sum / cnt) : 0;
}

/*
    R1 = 44000, R2 = 22000
    Vadc = Vbatt * (R2 / R1 + R2)
    -----------------------------
    adc         Vbatt      level
    -----------------------------
    280  under: no battery  -1
    3330 above: 8.4V
    3230      : 8.2V         4
    3130      : 8.0V
    3030      : 7.8V
    2930      : 7.6V         3
    2870      : 7.4V
    2780      : 7.2V
    2700      : 7.0V         2
    2610      : 6.8V
    2530      : 6.6V
    2450      : 6.4V         1
        under :              0
    -----------------------------
*/

int Hardware::checkBattery() {
    int      level;
    uint16_t adc = analogReadAvg(PIN_PWR_ADC);

    //LOG("BATT:%5d\n", adc);
    if (adc > 3230) {
        level = 4;
    } else if (adc > 2930) {
        level = 3;
    } else if (adc > 2700) {
        level = 2;
    } else if (adc > 2450) {
        level = 1;
    } else if (adc > 2000) {
        level = 0;
    } else {
        level = -1;
    }
    return level;
}
