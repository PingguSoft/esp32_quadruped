#include <FS.h>
#include <SPIFFS.h>
#include "CommandHandler.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
const float CommandHandler::_fPrevGain = MOTION_SMOOTH_GAIN;
const float CommandHandler::_fCurGain  = 1.0f - MOTION_SMOOTH_GAIN;


/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/


CommandHandler::CommandHandler(CommInterface* comm) :
    _pComm(comm),
    _quadruped(BODY_WIDTH, BODY_HEIGHT, BODY_COXA_LENGTH, BODY_COXA_OFFSET_Z, BODY_FEMUR_LENGTH, BODY_TIBIA_LENGTH),
    _gyroProc(HW_SERVO_UPDATE_FREQ)
{
    _nGait       = 0;
    _isWalk      = false;
    _isFlash     = false;
    _isBalance   = false;
    _isStand     = false;
    _isCamMove   = false;
    memset(_isConnected, 0, sizeof(_isConnected));

    _vecMove.zero();
    _rotMove.zero();

    _fCamPan  = 90;
    _fCamTilt = 90;
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::setup(void) {
    _hw.setup();
    _quadruped.setup(&_hw);
    _pGait = _gaitMan.get(0);
    _quadruped.setGait(_pGait);

#if CONFIG_ENABLE_GYRO
    _gyroProc.setup();
#endif

    loadUserData();
    _statusLed.setup();
    updateStatusLeds();
    _timedMove.setup(&_vecMove, &_rotMove);
    _timedMove.go({ 0, 0, BODY_IDLE_Z }, { 0, 0, 0 }, 1000);
}

void CommandHandler::setup(CommInterface* comm) {
    _pComm = comm;
    setup();
}

void CommandHandler::loop() {
    unsigned long ts = millis();
    Rotator r;

    if (_isStand) {
        if (_isWalk) {
            float mp  = Utils::mapf(_rc.pitch, 1000, 2000, -BODY_MAX_MOVE_MULT, BODY_MAX_MOVE_MULT);
            float mr  = Utils::mapf(_rc.roll,  1000, 2000, -BODY_MAX_MOVE_MULT, BODY_MAX_MOVE_MULT);

            _vecMove.x = (mp * _fCurGain) + (_vecMove.x * _fPrevGain);
            _vecMove.y = (mr * _fCurGain) + (_vecMove.y * _fPrevGain);
            if (!_isCamMove) {
                float my = Utils::mapf(_rc.yaw,      1000, 2000, -BODY_MAX_YAW,   BODY_MAX_YAW);
                my       = constrain(my * _fCurGain + _rotMove.yaw * _fPrevGain, -BODY_MAX_YAW,   BODY_MAX_YAW);

                float mt = Utils::mapf(_rc.throttle, 1000, 2000, -BODY_MAX_PITCH, BODY_MAX_PITCH);
                mt       = constrain(mt * _fCurGain + _rotMove.pitch * _fPrevGain, -BODY_MAX_PITCH, BODY_MAX_PITCH);

                _rotMove.set(my, mt, 0);
            }
        } else {        // pose
            if (!_isCamMove) {
                float my  = Utils::mapf(_rc.yaw,   1000, 2000, -BODY_MAX_YAW,   BODY_MAX_YAW);
                float mp  = Utils::mapf(_rc.pitch, 1000, 2000, -BODY_MAX_PITCH, BODY_MAX_PITCH);
                float mr  = Utils::mapf(_rc.roll,  1000, 2000, -BODY_MAX_ROLL, BODY_MAX_ROLL);
                _rotMove.set(my * _fCurGain + _rotMove.yaw   * _fPrevGain,
                             mp * _fCurGain + _rotMove.pitch * _fPrevGain,
                             mr * _fCurGain + _rotMove.roll  * _fPrevGain);
            }
        }
    }

    // for debug
    //
    if (!_vecMove.equals(_vecOldMove)) {
        //_vecMove.dump("DIR");
        _vecOldMove.set(_vecMove);
    }

    if (!_rotMove.equals(_rotOldMove)) {
        //_rotMove.dump("ROT");
        _rotOldMove.set(_rotMove);
    }
    //
    //

#if CONFIG_ENABLE_CAM_PAN_TILT
    if (_isCamMove) {
        _rotMove.zero();

        float fPanInc  = roundUp((1500 - _rc.yaw) / 100.0f);         // max 5 degree, (reverse direction)
        float fTiltInc = roundUp((_rc.throttle - 1500) / 100.0f);    // max 5 degree

        _fCamPan  = constrain(roundUp(_fCamPan + fPanInc),   10, 170);
        _fCamTilt = constrain(roundUp(_fCamTilt + fTiltInc), 80, 160);

        if (_fCamPan != _fOldCamPan || _fCamTilt != _fOldCamTilt) {
            getHW()->setPanTilt(_fCamPan * 10, _fCamTilt * 10);
            LOG("CAM : %6.1f, %6.1f\n", _fCamPan, _fCamTilt);
            _fOldCamPan  = _fCamPan;
            _fOldCamTilt = _fCamTilt;
        }
    }
#endif

#if CONFIG_ENABLE_GYRO
    r = _isBalance ? _gyroProc.process(ts, _rotMove) : _rotMove;
#else
    r = _rotMove;
#endif

    _timedMove.handle(ts);
    _quadruped.update(ts, _isWalk, &_vecMove, &r);
    updateStatusLeds();
    _statusLed.loop(ts);
}

bool CommandHandler::isSet(int shift, int state, int checkBtn) {
    int shiftMask  = checkBtn & (BTN_SHIFT_L | BTN_SHIFT_R);

    if (shiftMask != shift)
        return false;

    state &= ~(BTN_SHIFT_L | BTN_SHIFT_R);    // clear shift mask
    return bool(state & checkBtn);
}

void CommandHandler::setSwitches(int switches) {
    static int lastSwitches = 0;

    int  toggled   = (switches ^ lastSwitches);
    int  shift     = switches & (BTN_SHIFT_L | BTN_SHIFT_R);

    if (toggled == 0)
        return;

    lastSwitches = switches;
    if (isSet(shift, toggled, BTN_STAND)) {
        _isStand = (switches & BTN_STAND);
        if (_isStand) {
            _timedMove.go({ 0, 0, 0 }, { 0, 0, 0 }, 1000);
        } else {
            _timedMove.go({ 0, 0, BODY_IDLE_Z }, { 0, 0, 0 }, 1000);
        }
        LOG("stand:%d\n", int(_isStand));
    }

    if (isSet(shift, toggled, BTN_FLASH)) {
        _isFlash = (switches & BTN_FLASH);
        LOG("flash:%d\n", int(_isFlash));
    }

    if (isSet(shift, toggled, BTN_GAIT)) {
        _nGait = (_nGait + 1) % _gaitMan.getGaitCnt();
        _pGait = _gaitMan.get(_nGait);
        _quadruped.setGait(_pGait);
        LOG("gait : %s\n", _pGait->getName().c_str());
    }

    if (isSet(shift, toggled, BTN_WALK)) {
        _isWalk = (switches & BTN_WALK);
        LOG("walking : %d\n", int(_isWalk));
    }

    if (isSet(shift, toggled, BTN_OFFSET_DEC) || isSet(shift, toggled, BTN_OFFSET_DEC2)) {
        Vector *pOffset = _quadruped.getTipOffset();
        pOffset->x -= 5;
        LOG("tip offset : %6.1f\n", pOffset->x);
    }

    if (isSet(shift, toggled, BTN_OFFSET_INC) || isSet(shift, toggled, BTN_OFFSET_INC2)) {
        Vector *pOffset = _quadruped.getTipOffset();
        pOffset->x += 5;
        LOG("tip offset : %5.1f\n", pOffset->x);
    }

    if (isSet(shift, toggled, BTN_HEIGHT_DEC)) {
        if (_vecMove.z >= (BODY_MIN_Z + 5)) {
            _vecMove.z -= 5;
            _vecMove.dump("DIR");
        }
    }

    if (isSet(shift, toggled, BTN_HEIGHT_INC)) {
        if (_vecMove.z <= (BODY_MAX_Z - 5)) {
            _vecMove.z += 5;
            _vecMove.dump("DIR");
        }
    }

    if (isSet(shift, toggled, BTN_STEP_Z_DEC)) {
        if (_pGait && _pGait->getStep()->z > 0) {
            _pGait->getStep()->z--;
            _pGait->getStep()->dump("STEP");
        }
    }

    if (isSet(shift, toggled, BTN_STEP_Z_INC)) {
        if (_pGait && _pGait->getStep()->z < 50) {
            _pGait->getStep()->z++;
            _pGait->getStep()->dump("STEP");
        }
    }

    if (isSet(shift, toggled, BTN_STEP_INC)) {
        if (_pGait) {
            float steps = _pGait->getStepsPerSec() + 0.5f;
            if (steps < 100) {
                _pGait->setStepsPerSec(steps);
                LOG("steps per sec : %5.2f\n", steps);
            }
        }
    }

    if (isSet(shift, toggled, BTN_STEP_DEC)) {
        if (_pGait) {
            float steps = _pGait->getStepsPerSec() - 0.5f;
            if (steps > 0) {
                _pGait->setStepsPerSec(steps);
                LOG("steps per sec : %5.2f\n", steps);
            }
        }
    }

    if (isSet(shift, toggled, BTN_STEP_XY_DEC)) {
        if (_pGait->getStep()->x > 0) {
            _pGait->getStep()->x--;
        }
        if (_pGait->getStep()->y > 0) {
            _pGait->getStep()->y--;
        }
        if (_pGait) {
            _pGait->getStep()->dump("STEP");
        }
    }

    if (isSet(shift, toggled, BTN_STEP_XY_INC)) {
        if (_pGait->getStep()->x < 50) {
            _pGait->getStep()->x++;
        }
        if (_pGait->getStep()->y < 50) {
            _pGait->getStep()->y++;
        }
        if (_pGait) {
            _pGait->getStep()->dump("STEP");
        }
    }

    if (isSet(shift, toggled, BTN_BALANCE)) {
        _isBalance = (switches & BTN_BALANCE);
        LOG("balance mode:%d\n", int(_isBalance));
#if CONFIG_ENABLE_GYRO
        _gyroProc.reset();
#endif
    }

    if (isSet(shift, toggled, BTN_SAVE)) {
        saveUserData();
    }

    if (isSet(shift, toggled, BTN_LOAD)) {
        loadUserData();
    }

    //
    // camera related
    //
    if (isSet(shift, toggled, BTN_CAMERA_MOVE)) {
        _isCamMove = !_isCamMove;
        LOG("camera move : %d\n", int(_isCamMove));
    }

    if (isSet(shift, toggled, BTN_CAMERA_HOME)) {
        _fCamPan  = 90;
        _fCamTilt = 90;
    }
}

void CommandHandler::setButtons(int btns, bool toggle) {
    static int switches = 0;
    static int lastBtns = 0;

    int  shift     = btns & (BTN_SHIFT_L | BTN_SHIFT_R);
    int  others    = btns & ~(BTN_SHIFT_L | BTN_SHIFT_R);
    int  toggledOn = (others ^ lastBtns) & others;

    if (shift || toggledOn) {
        switches = switches ^ toggledOn;
        setSwitches(shift | switches);
    }
    lastBtns = toggle ? 0 : others;
}

void CommandHandler::onRC(struct param_rc* rc) {
    static unsigned long lastProtoCall;
    unsigned long ts = millis();

    if (rc->flag == CONTROLLER_PROTOCOL) {
#if 0
        LOG("y:%5d, t:%5d, r:%5d, p:%5d, ", rc->yaw, rc->throttle, rc->roll, rc->pitch);
        for (uint8_t i = 0; i < 8; i++) {
            LOG("A%d:%5d, ", i + 1, rc->aux[i]);
        }
        LOG("\n");
#endif
        if (!isConnected(CONTROLLER_PROTOCOL)) {
            setConnected(CONTROLLER_PROTOCOL, true);
        }
        lastProtoCall = ts;

        // btpad overrides control
        if (isConnected(CONTROLLER_BTPAD))
            return;

        setSwitches(aux2Switches(rc));
    }

    if (IS_ELAPSED(ts, lastProtoCall, 1000)) {
        if (isConnected(CONTROLLER_PROTOCOL)) {
            setConnected(CONTROLLER_PROTOCOL, false);
        }
    }

    _rc = *rc;
    if (rc->flag == CONTROLLER_BTPAD) {
        // dead zone
        _rc.pitch    = filterDeadZone(_rc.pitch);
        _rc.roll     = filterDeadZone(_rc.roll);
        _rc.throttle = filterDeadZone(_rc.throttle);
        _rc.yaw      = filterDeadZone(_rc.yaw);
    }
    //LOG("RC:%5d %5d %5d %5d\n", _rc.yaw, _rc.throttle, _rc.roll, _rc.pitch);
}

void CommandHandler::onAttitude(struct param_att* att) {
}

uint8_t CommandHandler::onBattDV(void) {
    return (uint8_t)0;
}

int8_t CommandHandler::onOthers(uint8_t cmd, uint8_t* pData, uint8_t size, uint8_t* pRes) {
    int8_t  ret = -1;

    switch (cmd) {
        case MSP_SET_ONLINE:
            LOG("RPi Online !!\n");
            break;

        case MSP_SET_OFFLINE:
            LOG("RPi Offline !!\n");
            break;
    }
    return ret;
}

void CommandHandler::saveUserData() {
    if (!_pGait)
        return;

    File file = SPIFFS.open(FILE_STEP_CFG, FILE_WRITE);
    if (!file) {
        LOG("file write open error\n");
        return;
    }
    file.write((uint8_t*)_pGait->getStep(), sizeof(Vector));

    // steps per sec
    float val = _pGait->getStepsPerSec();
    file.write((uint8_t*)&val, sizeof(float));

    // tip offset X
    val = _quadruped.getTipOffset()->x;
    file.write((uint8_t*)&val, sizeof(float));

    file.close();
    LOG("user data SAVED !!!\n");
}

void CommandHandler::loadUserData() {
    if (!_pGait)
        return;

    File file = SPIFFS.open(FILE_STEP_CFG);
    if (!file) {
        LOG("file not found\n");
        return;
    }

    if (file.read((uint8_t*)_pGait->getStep(), sizeof(Vector)) > 0) {
        _pGait->getStep()->dump("STEP");
    }

    float val;
    if (file.read((uint8_t*)&val, sizeof(float)) > 0) {
        val = round(val);
        _pGait->setStepsPerSec(val);
        LOG("setStepsPerSec:%5.2f\n", val);
    }

    if (file.read((uint8_t*)&val, sizeof(float)) > 0) {
        _quadruped.getTipOffset()->x = val;
        LOG("tipOffsetX:%5.2f\n", val);
    }

    file.close();
    LOG("user data LOADED !!!\n");
}

uint16_t CommandHandler::filterDeadZone(uint16_t val) {
    if (1480 <= val && val <= 1520) {
        val = 1500;
    }
    return val;
}

uint16_t CommandHandler::aux2CombBits(uint16_t val, uint8_t bits) {
    uint16_t div = BV(bits) - 1;

    val -= 1000;
    return val * div / 1000;
}

uint16_t CommandHandler::aux2Switches(struct param_rc* rc) {
    uint16_t btn  = 0;
    uint16_t comb = 0;

    for (uint8_t i = 0; i < 8; i++) {
        switch (i) {
            case 0:
                if (rc->aux[i] > 1500) {
                    btn |= _BV(ControlStick::BTN_L2);
                }
                break;

            case 1:
                if (rc->aux[i] > 1500) {
                    btn |= _BV(ControlStick::BTN_L1);
                }
                break;

            case 2:
                if (rc->aux[i] > 1500) {
                    btn |= _BV(ControlStick::BTN_R1);
                }
                break;

            case 3:
                if (rc->aux[i] > 1500) {
                    btn |= _BV(ControlStick::BTN_R2);
                }
                break;

            case 4:
                // aux5 :  button^0 button^1 switch4 switch5
                comb = aux2CombBits(rc->aux[i], 4);
                if (comb & 0x01) {
                    btn |= _BV(ControlStick::BTN_Y);
                }
                if (comb & 0x02) {
                    btn |= _BV(ControlStick::BTN_X);
                }
                if (comb & 0x04) {
                    btn |= _BV(ControlStick::BTN_START);
                }
                if (comb & 0x08) {
                    //btn |= _BV(ControlStick::BTN_LTHUMB);
                }
                break;

            case 5:
                // aux6 : button^2 button^3 switch6 switch7
                comb = aux2CombBits(rc->aux[i], 4);
                if (comb & 0x01) {
                    btn |= _BV(ControlStick::BTN_B);
                }
                if (comb & 0x02) {
                    btn |= _BV(ControlStick::BTN_A);
                }
                if (comb & 0x04) {
                }
                if (comb & 0x08) {
                    //btn |= _BV(ControlStick::BTN_RTHUMB);
                }
                break;

            case 6:
                if (rc->aux[i] < 1200) {
                    btn |= _BV(ControlStick::BTN_DPAD_LEFT);
                } else if (rc->aux[i] > 1800) {
                    btn |= _BV(ControlStick::BTN_DPAD_RIGHT);
                }
                break;

            case 7:
                if (rc->aux[i] < 1200) {
                    btn |= _BV(ControlStick::BTN_DPAD_DOWN);
                } else if (rc->aux[i] > 1800) {
                    btn |= _BV(ControlStick::BTN_DPAD_UP);
                }
                break;
        }
    }
    return btn;
}

void CommandHandler::updateStatusLeds() {
    static int iOldState = -1;
    static int iOldBlink = -1;

    const StatusLed::color_idx_t colorStatus[] = {
        StatusLed::COLOR_BLUE,          // bt connecting
        StatusLed::COLOR_LIGHTBLUE,     // no walk
        StatusLed::COLOR_PURPLE,        // trot
        StatusLed::COLOR_GREEN,         // lateral
        StatusLed::COLOR_CYAN,          // diagonal
    };

    int  state;
    int  blink = 0;
    bool isConnected = _isConnected[0] | _isConnected[1];
    StatusLed::color_idx_t  offBlink = StatusLed::COLOR_BLACK;

    if (isConnected) {
        if (!_isWalk) {
            state = 1;
        } else {
            state = 2 + _nGait;
        }
        if (_isBalance) {
            blink = 500;
            offBlink = StatusLed::COLOR_PINK;
        } else if (_isFlash) {
            blink = 200;
            offBlink = StatusLed::COLOR_BLACK;
        }
    } else {
        state = 0;
        blink = 500;
    }

    if (state != iOldState || blink != iOldBlink) {
        _statusLed.set(StatusLed::ALL_POS, colorStatus[state], blink, offBlink);
        iOldState = state;
        iOldBlink = blink;
    }
}
