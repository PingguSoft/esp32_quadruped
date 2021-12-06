#ifndef _COMMAND_HANDLER_H_
#define _COMMAND_HANDLER_H_

#include "config.h"
#include "CommInterface.h"
#include "QuadRuped.h"
#include "StatusLed.h"
#include "ControlStick.h"
#include "TimedMove.h"
#include "utils.h"
#include "GyroProc.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
typedef enum {
    CONTROLLER_PROTOCOL = 0,
    CONTROLLER_BTPAD,
    CONTROLLER_MAX,
} controller_t;

#define BTN_SHIFT_L         _BV(ControlStick::BTN_L2)
#define BTN_SHIFT_R         _BV(ControlStick::BTN_R2)

#define BTN_WALK            _BV(ControlStick::BTN_A)
#define BTN_GAIT            _BV(ControlStick::BTN_B)
#define BTN_BALANCE         _BV(ControlStick::BTN_X)
#define BTN_FLASH           _BV(ControlStick::BTN_Y)
#define BTN_STAND           _BV(ControlStick::BTN_START)
#define BTN_OFFSET_DEC      _BV(ControlStick::BTN_DPAD_LEFT)
#define BTN_OFFSET_INC      _BV(ControlStick::BTN_DPAD_RIGHT)
#define BTN_HEIGHT_DEC      (_BV(ControlStick::BTN_L1) | _BV(ControlStick::BTN_DPAD_DOWN))
#define BTN_HEIGHT_INC      (_BV(ControlStick::BTN_R1) | _BV(ControlStick::BTN_DPAD_UP))
#define BTN_SAVE            (_BV(ControlStick::BTN_LTHUMB) | _BV(ControlStick::BTN_MENU))
#define BTN_LOAD            (_BV(ControlStick::BTN_RTHUMB) | _BV(ControlStick::BTN_POWER))

// with SHIFT_L
#define BTN_STEP_DEC        (BTN_SHIFT_L | _BV(ControlStick::BTN_L1))
#define BTN_STEP_INC        (BTN_SHIFT_L | _BV(ControlStick::BTN_R1))
#define BTN_STEP_Z_DEC      (BTN_SHIFT_L | _BV(ControlStick::BTN_X))
#define BTN_STEP_Z_INC      (BTN_SHIFT_L | _BV(ControlStick::BTN_Y))
#define BTN_STEP_XY_DEC     (BTN_SHIFT_L | _BV(ControlStick::BTN_A))
#define BTN_STEP_XY_INC     (BTN_SHIFT_L | _BV(ControlStick::BTN_B))

// with SHIFT_R
#define BTN_OFFSET_DEC2     (BTN_SHIFT_R | _BV(ControlStick::BTN_L1))
#define BTN_OFFSET_INC2     (BTN_SHIFT_R | _BV(ControlStick::BTN_R1))
#define BTN_CAMERA_MOVE     (BTN_SHIFT_R | _BV(ControlStick::BTN_A))

// with SHIFT_L and SHIFT_R
#define BTN_CAMERA_HOME     (BTN_SHIFT_L | BTN_SHIFT_R | _BV(ControlStick::BTN_A))

/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/

//
// CommSerial2
//
class CommSerial2 : public CommInterface {
private:
    class Serial2Dev : public ProtocolDevice {
        public:
            Serial2Dev() {
                // rpi txd --- esp     rxd
                // rpi rxd --- esp     txd
                Serial2.begin(115200, SERIAL_8N1, PIN_RXD2, PIN_TXD2);
            }

            virtual ~Serial2Dev() {
            }

            virtual void write(u8 *pBuf, u8 size) {
                Serial2.write(pBuf, size);
            }

            virtual int read(void) {
                return Serial2.read();
            }

            virtual int available(void) {
                return Serial2.available();
            }

        private:
    };

public:
    CommSerial2(int arg) : CommInterface(arg) {
        _pDev = new Serial2Dev();
        getProtocol()->setDevice(_pDev);
        setConnected(TRUE);
    }

    virtual int loop(void) {
        getProtocol()->processRx();
        return 1;
    }

private:
    Serial2Dev *_pDev;
    uint8_t  _ctr = 0;
    uint8_t  _buf[100];
};

class CommandHandler : public ProtocolCallback {
public:
    CommandHandler(CommInterface* comm = NULL);
    ~CommandHandler();

    Hardware  *getHW()          {   return &_hw;        }
    StatusLed *getStatusLed()   {   return &_statusLed; }
    QuadRuped *getSpot()        {   return &_quadruped; }
    GyroProc  *getGyroProc()    {   return &_gyroProc;  }

    void setup(void);
    void setup(CommInterface* comm);
    void loop();

    // joystick related functions
    void setConnected(controller_t con, bool connect)   {   _isConnected[con] = connect;    };
    bool isConnected(controller_t con)                  {   return _isConnected[con];       }
    void setSwitches(int btns);
    void setButtons(int btns, bool toggle = false);
    void toggleButtons(int btns) { setButtons(btns, true); }

    // virtual functions
    virtual void    onRC(struct param_rc* rc);
    virtual void    onAttitude(struct param_att* att);
    virtual uint8_t onBattDV(void);
    virtual int8_t  onOthers(uint8_t cmd, uint8_t* pData, uint8_t size, uint8_t* pRes);

private:
    bool isSet(int shift, int toggledOn, int checkBtn);
    uint16_t filterDeadZone(uint16_t val);
    uint16_t aux2Switches(struct param_rc* rc);
    uint16_t aux2CombBits(uint16_t val, uint8_t bits);

    void saveUserData();
    void loadUserData();
    void updateStatusLeds();

    float roundUp(float v)  { return round(v * 10.0f) / 10.0f; }

private:
    CommInterface* _pComm;

    int             _nGait;
    Vector          _vecMove;
    Vector          _vecOldMove;
    Rotator         _rotMove;
    Rotator         _rotOldMove;

    static const float _fCurGain;
    static const float _fPrevGain;

    bool            _isConnected[CONTROLLER_MAX];
    bool            _isStand;
    bool            _isWalk;
    bool            _isFlash;
    bool            _isBalance;
    bool            _isCamMove;

    struct param_rc _rc;

    Hardware        _hw;
    Gait           *_pGait;
    GaitMan         _gaitMan;
    QuadRuped       _quadruped;
    TimedMove       _timedMove;

    StatusLed       _statusLed;
    GyroProc        _gyroProc;

    float           _fCamPan;
    float           _fOldCamPan;
    float           _fCamTilt;
    float           _fOldCamTilt;
};

#endif