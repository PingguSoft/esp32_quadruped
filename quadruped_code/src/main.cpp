#include <Wire.h>
#include <arduino-timer.h>
#include "CommInterface.h"
#include "ControlStick.h"
#include "OTA.h"
#include "FSBrowser.h"
#include "CommandHandler.h"
#include "utils.h"

using namespace std::placeholders;

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/

/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/
#if (CONFIG_CONTROL == CONFIG_CONTROL_BTPADS)
static ControlStick         _joy;
#endif

static CommInterface*       _pComm = NULL;
static CommandHandler       _cmdHandler;

static OTA                  _ota(SERVER_NAME);
static FSBrowser            _fsbrowser(SERVER_NAME);
static Timer<>              _timer = timer_create_default();

static bool                 _isCal = false;

/*
***************************************************************************************************
* PROTOCOL
***************************************************************************************************
*/


/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
int loopOTA(unsigned long ts) {
    static int state = 0;

    switch (state) {
        case 0:
#if CONFIG_ENABLE_BTPADS
            _joy.stop();
#endif
            _ota.setup((char*)WIFI_SSID, (char*)WIFI_PASSWORD);
            _cmdHandler.getStatusLed()->set(StatusLed::ALL_POS, StatusLed::COLOR_RED, 1000);
            state = 1;
            return -1;

        case 1:
            _ota.loop();
            _cmdHandler.getStatusLed()->loop(ts);
            return -1;
    }
    return 0;
}

int loopFS(unsigned long ts, bool isClose = false) {
    static int state = 0;

    if (isClose) {
        state = 2;
    }

    switch (state) {
        case 0:
            state = 1;
            _fsbrowser.setup((char*)WIFI_SSID, (char*)WIFI_PASSWORD);
            _cmdHandler.getStatusLed()->set(StatusLed::ALL_POS, StatusLed::COLOR_YELLOW, 1000);
            return -1;

        case 1:
            _fsbrowser.loop();
            _cmdHandler.getStatusLed()->loop(ts);
            return -1;

        case 2:
            _fsbrowser.close();
            state = 0;
            return -1;

    }
    return 0;
}

/*
***************************************************************************************************
* Bluetooth Stick callback
***************************************************************************************************
*/
class StickCB : public StickCallback {
private:
    struct param_rc _rc;
    const uint8_t   _kTblDpadMap[9];

public:
    StickCB() :
        _rc {
            1500, 1500, 1500, 1500,
            1000, 1000, 1000, 1000, 1000,
            1000, 1000, 1000, 1000, 1000,
            1
        },
        _kTblDpadMap {
            0x00,
            0x01,   // up
            0x03,
            0x02,   // right
            0x06,
            0x04,   // down
            0x0c,
            0x08,   // left
            0x09
        }
    {
    }

    void onConnect() {
        _cmdHandler.setConnected(CONTROLLER_BTPAD, true);
    }

    void onDisconnect() {
        _cmdHandler.setConnected(CONTROLLER_BTPAD, false);
    }

    void onStickChanged(int axisX, int axisY, int axisZ, int axisRZ, int axisLT, int axisRT, int dpad, int btns) {
        _rc.yaw = map(axisX, 0, 1024, 1000, 2000);
        _rc.throttle = map(axisY, 0, 1024, 2000, 1000);
        _rc.roll = map(axisZ, 0, 1024, 1000, 2000);
        _rc.pitch = map(axisRZ, 0, 1024, 2000, 1000);
        _cmdHandler.onRC(&_rc);

        uint8_t  dp      = (dpad > 8) ? 0 : _kTblDpadMap[dpad];
        uint32_t btn     = (int(dp) << 16) | btns;
        _cmdHandler.setButtons(btn);
    }
};

/*
***************************************************************************************************
* Serial Command
***************************************************************************************************
*/
static int handleSerialCommand(unsigned long ts, int key) {
    static int  iMode = 0;
    static struct param_rc rc = { 1500, 1500, 1500, 1500,
                                  1000, 1000, 1000, 1000, 1000,
                                  1000, 1000, 1000, 1000, 1000,
                                  1 };

    switch (key) {
        case 'm':
            if (iMode == 0 || iMode == 1) {
                iMode = (iMode == 0) ? 1 : 0;
                if (iMode > 0) {
                    LOG("\n\n>>> calibration mode !!!\n");
                } else {
                    LOG("\n\n>>> normal mode !!!\n");
                }
            }
            break;

        case '&':
            if (iMode == 0 || iMode == 2) {
                iMode = (iMode == 0) ? 2 : 0;
                if (iMode > 0) {
                    LOG("\n\n>>> fs browser mode !!!\n");
                } else {
                    LOG("\n\n>>> normal mode !!!\n");
                    loopFS(ts, true);
                }
            }
            break;

        case '*':
            if (iMode == 0 || iMode == 3) {
                iMode = (iMode == 0) ? 3 : 0;
                if (iMode > 0) {
                    LOG("\n\n>>> OTA mode !!!\n");
                } else {
                    LOG("\n\n>>> normal mode !!!\n");
                }
            }
            break;
    }

    if (iMode > 0) {
        switch (iMode) {
            case 1:
                _cmdHandler.getHW()->calibrateLegs(key);
                return -100;

            case 2:
                loopFS(ts);
                return -100;

            case 3:
                loopOTA(ts);
                return -100;
        }
    }

    //
    // robot control
    //
    switch (key) {
        case 'w':
            if (rc.pitch < 2000) {
                rc.pitch++;
                _cmdHandler.onRC(&rc);
            }
            break;

        case 's':
            if (rc.pitch > 1000) {
                rc.pitch--;
                _cmdHandler.onRC(&rc);
            }
            break;

        case 'a':
            if (rc.roll > 1000) {
                rc.roll--;
                _cmdHandler.onRC(&rc);
            }
            break;

        case 'd':
            if (rc.roll < 2000) {
                rc.roll++;
                _cmdHandler.onRC(&rc);
            }
            break;

        case 'z':
            _cmdHandler.toggleButtons(BTN_HEIGHT_DEC);
            break;

        case 'c':
            _cmdHandler.toggleButtons(BTN_HEIGHT_INC);
            break;

        case 'e':
            if (rc.yaw < 2000) {
                rc.yaw += 10;
                _cmdHandler.onRC(&rc);
            }
            break;

        case 'q':
            if (rc.yaw > 1000) {
                rc.yaw -= 10;
                _cmdHandler.onRC(&rc);
            }
            break;

        case '1':
            _cmdHandler.toggleButtons(BTN_WALK);
            break;

        case '2':
            _cmdHandler.toggleButtons(BTN_GAIT);
            break;

        case '3':
            _cmdHandler.toggleButtons(BTN_BALANCE);
            //rc.roll = rc.pitch = rc.throttle = rc.yaw = 1500;
            break;

        case '4':
            _cmdHandler.toggleButtons(BTN_FLASH);
            //rc.roll = rc.pitch = rc.throttle = rc.yaw = 1500;
            break;

        case '9':
            _cmdHandler.toggleButtons(BTN_SAVE);
            break;

        case '0':
            _cmdHandler.toggleButtons(BTN_LOAD);
            break;

        case '!':
            _cmdHandler.getSpot()->setDebugMask(_cmdHandler.getSpot()->getDebugMask() ^ BV(0));
            break;

        case '@':
            _cmdHandler.getSpot()->setDebugMask(_cmdHandler.getSpot()->getDebugMask() ^ BV(1));
            break;

        case '#':
            _cmdHandler.getSpot()->setDebugMask(_cmdHandler.getSpot()->getDebugMask() ^ BV(2));
            break;

        case '$':
            _cmdHandler.getSpot()->setDebugMask(_cmdHandler.getSpot()->getDebugMask() ^ BV(3));
            break;

        case '=':
            _cmdHandler.toggleButtons(BTN_STEP_DEC);
            break;

        case '-':
            _cmdHandler.toggleButtons(BTN_STEP_INC);
            break;

        case '[':
            _cmdHandler.toggleButtons(BTN_STEP_XY_DEC);
            break;

        case ']':
            _cmdHandler.toggleButtons(BTN_STEP_XY_INC);
            break;

        case ';':
            _cmdHandler.toggleButtons(BTN_STEP_Z_DEC);
            break;

        case '\'':
            _cmdHandler.toggleButtons(BTN_STEP_Z_INC);
            break;

        case ' ':
            _cmdHandler.toggleButtons(BTN_STAND);
            break;

        case ',':
            _cmdHandler.toggleButtons(BTN_OFFSET_DEC);
            break;

        case '.':
            _cmdHandler.toggleButtons(BTN_OFFSET_INC);
            break;

        case 'h':
            _cmdHandler.getHW()->dump();
            break;

        //
        // gyro proc
        //
        case 'p':
            _cmdHandler.getGyroProc()->decP();
            break;

        case 'P':
            _cmdHandler.getGyroProc()->incP();
            break;

        case 'i':
            _cmdHandler.getGyroProc()->decI();
            break;

        case 'I':
            _cmdHandler.getGyroProc()->incI();
            break;

        case 'o':
            _cmdHandler.getGyroProc()->decD();
            break;

        case 'O':
            _cmdHandler.getGyroProc()->incD();
            break;

        case 'u':
            _cmdHandler.getGyroProc()->togglePID();
            break;

        case 'U':
            _cmdHandler.getGyroProc()->calibrate();
            break;
    }
    return key;
}

/*
***************************************************************************************************
* Setup
***************************************************************************************************
*/
static bool batteryWarning(void* param) {
    if (int(param) >= 0) {
        LOG("battery warning : %d\n", int(param));
    }
    int level = digitalRead(PIN_PWR_LED);
    digitalWrite(PIN_PWR_LED, !level);

    return true;
}

static bool batteryCheck(void* param) {
    static int          lastLevel = 5;
    static uintptr_t    taskBattLed = 0;
    static const int    tblDelays[] = {
        2000,   // -1 : no batt
        200,    //  0
        500     //  1
    };

    int level = _cmdHandler.getHW()->checkBattery();
    if (lastLevel != level) {
        if (taskBattLed) {
            _timer.cancel(taskBattLed);
        }
        if (level < 2) {
            taskBattLed = _timer.every(tblDelays[level + 1], batteryWarning, (void*)level);
        } else {
            digitalWrite(PIN_PWR_LED, HIGH);
        }
        lastLevel = level;
    }

    return true;
}

void setup() {
    setCpuFrequencyMhz(80);
    Serial.begin(115200);
    LOG("setup start !!! : heap:%d, psram:%d\n", ESP.getFreeHeap(), ESP.getPsramSize());

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_PWR_LED, OUTPUT);
    digitalWrite(PIN_PWR_LED, HIGH);

#ifdef PIN_CAL_SW
    pinMode(PIN_CAL_SW, INPUT_PULLUP);
    _isCal = (digitalRead(PIN_CAL_SW) == LOW);
#endif

    pinMode(PIN_SCL0, INPUT_PULLUP);
    pinMode(PIN_SDA0, INPUT_PULLUP);
    Wire.setPins(PIN_SDA0, PIN_SCL0);
    Wire.begin();
    Wire.setClock(400000);

    if (!SPIFFS.begin(true)) {
        LOG("SPIFFS Mount Failed\n");
    }
    _timer.every(5000, batteryCheck);

    if (!_isCal) {
#if (CONFIG_CONTROL == CONFIG_CONTROL_BTPADS)
        _joy.setStickCallback(new StickCB());
        _joy.addSupportedDevices();
        _joy.begin();
        _pComm = new CommSerial2(128);
#elif (CONFIG_CONTROL == CONFIG_CONTROL_BT_UART)
        _pComm = new CommSerialBT((char*)SERVER_NAME, 128);
#endif
    }

    if (_pComm) {
        _pComm->getProtocol()->setCallback(&_cmdHandler);
        //_pComm->getProtocol()->setSwButtonCmd();
    }
    _cmdHandler.setup(_pComm);
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_PWR_LED, LOW);
        delay(300);
        digitalWrite(PIN_PWR_LED, HIGH);
        delay(300);
    }
    LOG("setup finished !!!\n");
}

/*
***************************************************************************************************
* loop
***************************************************************************************************
*/
void loop() {
    unsigned long ts = millis();

    //
    // main loop
    //
    int key = Serial.read();

    if (_isCal) {
        _cmdHandler.getHW()->calibrateLegs(key);
    } else {
        key = handleSerialCommand(ts, key);
        if (key != -100) {
            _cmdHandler.loop();
        }
#if (CONFIG_CONTROL == CONFIG_CONTROL_BTPADS)
        if (_joy.isConnecting()) {
            _joy.connect();
        }
        if (!_joy.isConnected())
#endif
            _pComm->loop();
    }

    _timer.tick();
}
