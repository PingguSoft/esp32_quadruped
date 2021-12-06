#ifndef _CONTROLSTICK_H_
#define _CONTROLSTICK_H_

#include <Arduino.h>
#include <NimBLEDevice.h>

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
class StickCallback {
public:
    StickCallback()             {}
    virtual void onConnect()    {}
    virtual void onDisconnect() {}
    virtual void onStickChanged(int axisX, int axisY, int axisZ, int axisRZ, int axisLT, int axisRT, int dpad, int btns)    {}
};


class AdvertisedDeviceCallbacks;
class BLEJoyClientCallback;

class ControlStick {
public:
    enum {
        BTN_A = 0,
        BTN_B,
        BTN_MENU,
        BTN_X,
        BTN_Y,
        BTN_RSV1,
        BTN_L1,
        BTN_R1,

        BTN_L2 = 8,
        BTN_R2,
        BTN_SEL,
        BTN_START,
        BTN_POWER,
        BTN_LTHUMB,
        BTN_RTHUMB,
        BTN_RSV3,

        BTN_DPAD_UP = 16,
        BTN_DPAD_RIGHT,
        BTN_DPAD_DOWN,
        BTN_DPAD_LEFT
    } BTN_T;

    ControlStick();
    virtual ~ControlStick() { }

    void add(BLEAddress* addr, BLEUUID uuidService, BLEUUID uuidCharacteristic, notify_callback cb);
    void begin();
    bool connect();
    bool isConnecting() { return _isConnecting; }
    bool isConnected()  { return _isConnected; }
    void rescan();
    void stop();

    void addSupportedDevices();
    void setStickCallback(StickCallback *callback)   { _pStickCallback = callback; }

    friend class AdvertisedDeviceCallbacks;
    friend class BLEJoyClientCallback;

protected:
    std::vector <BLEAddress*>       _reqDevAddrs;
    std::vector <BLEUUID>           _uuidServices;
    std::vector <BLEUUID>           _uuidCharacteristics;
    std::vector <notify_callback>   _notifyCallbacks;

    BLEAddress*                 _pServerAddress;
    boolean                     _isConnecting;
    boolean                     _isConnected;
    BLERemoteCharacteristic*    _pRemoteCharacteristic;

    StickCallback*              _pStickCallback;

    void cbControlStickBLE(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void cbFlyPadBLE(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void cbGameSirT1DBLE(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);


private:
    int getBit(int val, int bit) { return ((val >> bit) & 1); }
    int setBit(int val, int bit) { return (val << bit);       }

    BLEUUID                     _uuidFoundService;
    int                         _nFoundIdx;
    int                         _nConnTryCtr;
    NimBLEClient*               _pClient;
};

#if 0
class JoystickBLE : public ControlStick {
public:
    // this is the service UUID of the VR Control handheld mouse/joystick device (HID)
    //"00001812-0000-1000-8000-00805f9b34fb"

    // this characteristic UUID works for joystick & triggers (report)
    //"00002A4D-0000-1000-8000-00805f9b34fb"
    JoystickBLE(BLEUUID uuidService = BLEUUID("1812"), BLEUUID uuidCharacteristic = BLEUUID("2A4D")) : ControlStick(uuidService, uuidCharacteristic) {
    }

    ~JoystickBLE() {
    }
};

class FlyPadBLE : public ControlStick {
public:
    FlyPadBLE(BLEUUID uuidService = BLEUUID("9e35fa00-4344-44d4-a2e2-0c7f6046878b"), BLEUUID uuidCharacteristic = BLEUUID("9e35fa01-4344-44d4-a2e2-0c7f6046878b")) : ControlStick(uuidService, uuidCharacteristic) {
    }

    ~FlyPadBLE() {
    }
};
#endif

#endif