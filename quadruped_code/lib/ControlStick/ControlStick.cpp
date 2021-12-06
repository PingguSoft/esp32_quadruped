#include "ControlStick.h"

using namespace std::placeholders;

#ifdef  LOG
#undef  LOG
#endif

#define LOG(...)    //printf(__VA_ARGS__)

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
* VARIABLES
*****************************************************************************************
*/


/*
*****************************************************************************************
* FUNCTIONS
*****************************************************************************************
*/
ControlStick::ControlStick() {
    _pStickCallback = nullptr;
}

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    ControlStick* _pJoy;

public:
    AdvertisedDeviceCallbacks(ControlStick* pJoy) : NimBLEAdvertisedDeviceCallbacks() {
        _pJoy = pJoy;
    }

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        //Found a device, check if the service is contained and optional if address fits requested address
        LOG("advertised device: %s\n", advertisedDevice->toString().c_str());

        for (int i = 0; i < _pJoy->_uuidServices.size(); i++) {
            if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().equals(_pJoy->_uuidServices[i])) {
                if (_pJoy->_reqDevAddrs[i] == nullptr || (_pJoy->_reqDevAddrs[i] && advertisedDevice->getAddress().equals(*_pJoy->_reqDevAddrs[i]))) {
                    LOG("found : %s\n", advertisedDevice->getName().c_str());
                    _pJoy->_nFoundIdx        = i;
                    _pJoy->_nConnTryCtr      = 0;
                    _pJoy->_uuidFoundService = advertisedDevice->getServiceUUID();
                    _pJoy->_pServerAddress   = new BLEAddress(advertisedDevice->getAddress());
                    _pJoy->_isConnecting     = true;
                    advertisedDevice->getScan()->stop();
                    break;
                }
            }
        }
    }
};

class BLEJoyClientCallback : public NimBLEClientCallbacks {
    ControlStick* _pJoy;

public:
    BLEJoyClientCallback(ControlStick* pJoy) : NimBLEClientCallbacks() {
        _pJoy = pJoy;
    }

    void onDisconnect(BLEClient* bleClient) {
        _pJoy->_isConnecting = false;
        _pJoy->_isConnected = false;
        LOG("disconnected client\n");
        if (_pJoy && _pJoy->_pStickCallback)
            _pJoy->_pStickCallback->onDisconnect();

        if (_pJoy->_pServerAddress) {
            delete _pJoy->_pServerAddress;
            _pJoy->_pServerAddress = nullptr;
        }

        if (_pJoy->_pClient) {
            //NimBLEDevice::deleteClient(_pJoy->_pClient);
            //_pJoy->_pClient = nullptr;
        }
        _pJoy->rescan();
    }
};

bool ControlStick::connect() {
    if (_pServerAddress == nullptr) {
        return false;
    }

    BLEAddress pAddress = *_pServerAddress;
    _nConnTryCtr++;
    LOG("TRY:%2d - number of ble clients: %d\n", _nConnTryCtr, NimBLEDevice::getClientListSize());

    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getClientListSize()) {
        /** Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        _pClient = NimBLEDevice::getClientByPeerAddress(pAddress);
        if (!_pClient) {
            /** We don't already have a client that knows this device,
             *  we will check for a client that is disconnected that we can use.
             */
            _pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    if (!_pClient) {
        if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            LOG("max clients reached - no more connections available: %d", NimBLEDevice::getClientListSize());
        } else {
            /** No client to reuse? Create a new one. */
            _pClient = NimBLEDevice::createClient();
        }
    }

    bool ret = false;
    if (_pClient) {
        ret = _pClient->isConnected();
        if (!ret) {
            ret = _pClient->connect(pAddress, true);
        }
    }

    if (ret) {
        // add callback instance to get notified if a disconnect event appears
        _pClient->setClientCallbacks(new BLEJoyClientCallback(this));

        std::vector<NimBLERemoteService*> *pSvcs = _pClient->getServices(true);
        std::vector<NimBLERemoteService*> svcs = *pSvcs;
        LOG("services :%d\n", svcs.size());
        for(NimBLERemoteService *svc: svcs) {
            LOG("services:%s\n", svc->toString().c_str());
        }

        LOG("connected to: %s, RSSI: %d, uuid:%s\n", _pClient->getPeerAddress().toString().c_str(), _pClient->getRssi(), _uuidFoundService.toString().c_str());
        BLERemoteService* pRemoteService = _pClient->getService(_uuidFoundService);

        if (pRemoteService) {
            std::vector<NimBLERemoteCharacteristic*> *pChars = pRemoteService->getCharacteristics(_nConnTryCtr > 2);
            if (pChars) {
                std::vector<NimBLERemoteCharacteristic*> chars = *pChars;

                // logs
                LOG("get service success chars:%d\n", chars.size());
                if (_nConnTryCtr > 2) {
                    for(NimBLERemoteCharacteristic *it: chars) {
                        LOG("chars:%s\n", it->getUUID().toString().c_str());
                    }
                }

                _pRemoteCharacteristic = pRemoteService->getCharacteristic(_uuidCharacteristics[_nFoundIdx]);
                if (_pRemoteCharacteristic) {
                    if (_pRemoteCharacteristic->canNotify()) {
                        LOG("notifiable.. subscribed!! : %s\n", _pRemoteCharacteristic->getUUID().toString().c_str());
                        _pRemoteCharacteristic->subscribe(true, _notifyCallbacks[_nFoundIdx], true);
                    }

                    if (_pStickCallback)
                        _pStickCallback->onConnect();

                    _isConnected = true;
                    _isConnecting = false;
                    ret = true;
                } else {
                    LOG("failed to get ble service\n");
                }
            } else {
                LOG("no characteristics found\n");
            }
        } else {
            LOG("no remote service found\n");
        }
    } else {
        LOG("failed to connect\n");
    }
    return ret;
}

static ControlStick *_pParent;
static void scanEndedCallback(NimBLEScanResults results) {
/*
    LOG("Number of devices: %d\n", results.getCount());
    for (int i = 0; i < results.getCount(); i++) {
        LOG("device[%d]: %s\n", i, results.getDevice(i).toString().c_str());
    }
*/
    LOG("SCAN ENDED\n");
    _pParent->rescan();
}

void ControlStick::rescan() {
    BLEScan* pBLEScan = BLEDevice::getScan();
    if (!isConnecting() && !isConnected()) {
        LOG("SCAN AGAIN !!!\n");
        pBLEScan->setActiveScan(true);
        pBLEScan->start(5, scanEndedCallback);
    }
}

void ControlStick::begin() {
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();

    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks(this));
    pBLEScan->setActiveScan(true);
    _pParent = this;
    pBLEScan->start(5, scanEndedCallback);
}

void ControlStick::stop() {
    if (!BLEDevice::getInitialized())
        return;

    BLEScan* pBLEScan = BLEDevice::getScan();
    if (pBLEScan) {
        pBLEScan->stop();
    }
    BLEDevice::deinit(true);
}

void ControlStick::add(BLEAddress* addr, BLEUUID uuidService, BLEUUID uuidCharacteristic, notify_callback cb) {
    _reqDevAddrs.push_back(addr);
    _uuidServices.push_back(uuidService);
    _uuidCharacteristics.push_back(uuidCharacteristic);
    _notifyCallbacks.push_back(cb);
}

/*
***************************************************************************************************
* Callbacks
***************************************************************************************************
*/
void ControlStick::addSupportedDevices() {
    // PG-9167
    add(nullptr, BLEUUID("1812"), BLEUUID("2A4D"), std::bind(&ControlStick::cbControlStickBLE, this, _1, _2, _3, _4));

    // flypad
    add(nullptr, BLEUUID("9e35fa00-4344-44d4-a2e2-0c7f6046878b"),
        BLEUUID("9e35fa01-4344-44d4-a2e2-0c7f6046878b"), std::bind(&ControlStick::cbFlyPadBLE, this, _1, _2, _3, _4));

    // gamesir-t1d
    add(nullptr, BLEUUID("00008650-0000-1000-8000-00805f9b34fb"),
        BLEUUID("00008651-0000-1000-8000-00805f9b34fb"), std::bind(&ControlStick::cbGameSirT1DBLE, this, _1, _2, _3, _4));
}

void ControlStick::cbControlStickBLE(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    //             0  1  2  3  4  5  6  7  8  9
    // 00000000 - 80 80 80 80 ff 00 00 00 00 e2
    //            LA LA RA RA dp B1 B2
    //            LR UD LR UD
    //
    //            7    6    5    4    3    2    1    0
    // B1  bits: R1   L1         Y    X         B    A
    // B2  bits:      RTH  LTH      START SEL  R2   L2
    // dp   val: none-0xff up-0, r-1, d-2, l-3

    int btns = (int(pData[6]) << 8) | int(pData[5]);
    int dpad = pData[4] + 1;

    // 8bits -> 10bits
    if (_pStickCallback)
        _pStickCallback->onStickChanged(int(pData[0]) << 2, int(pData[1]) << 2, int(pData[2]) << 2, int(pData[3]) << 2, 0, 0, dpad, btns);
}

void ControlStick::cbFlyPadBLE(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    //             0  1  2  3  4  5  6
    // 00000000 - 50 00 00 80 80 80 80
    //               B1 B2 RA RA LA LA
    //                     LR UP LR UP
    //
    //            7    6    5    4    3    2    1    0
    // B1  bits: L1   R2   R1    A    B    2    1  TAKEOFF
    // B2  bits:                          RTH  LTH   L2
    int btns = (int(pData[2]) << 8) | (pData[1] & 0xff);
    btns =  setBit(getBit(btns,  0), BTN_START)  |
            setBit(getBit(btns,  1), BTN_X)      |
            setBit(getBit(btns,  2), BTN_Y)      |
            setBit(getBit(btns,  3), BTN_B)      |
            setBit(getBit(btns,  4), BTN_A)      |
            setBit(getBit(btns,  5), BTN_R1)     |
            setBit(getBit(btns,  6), BTN_R2)     |
            setBit(getBit(btns,  7), BTN_L1)     |
            setBit(getBit(btns,  8), BTN_L2)     |
            setBit(getBit(btns,  9), BTN_LTHUMB) |
            setBit(getBit(btns, 10), BTN_RTHUMB);

    // 8bits -> 10bits
    if (_pStickCallback)
        _pStickCallback->onStickChanged(int(pData[5]) << 2, int(pData[6]) << 2, int(pData[3]) << 2, int(pData[4]) << 2, 0, 0, 0, btns);
}

void ControlStick::cbGameSirT1DBLE(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    //           0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
    //          a1 c5 80 20 08 02 00 00 00 00 00 00 00 00 00 00 00 00 00 9b
    //                               L2 R2 B1 B2 DP
    //                                A  A
    // L2 Anal :                     XX                                          8bit
    // R2 Anal :                        XX                                       8bit
    // L-LR    :      XX X                                                      10bit
    // L-UPDOWN:         XX X                                                   10bit
    // R-LR    :            XX X                                                10bit
    // R-UPDOWN:               XX X                                             10bit
    //
    //            7    6    5    4    3    2    1    0
    // B1 bits : R1   L1         Y    X   menu  B    A
    // B2 bits :     RTH  LTH  PWR   C2   C1   R2   L2
    // DP val  : none-0, up-1, ur-2, r-3, rd-4, d-5, dl-6, l-7, ul-8

    if (length < 12 || pData[0] != (byte)0xa1 || pData[1] != (byte)0xc5) {
        return;
    }

    // 10 bits * 4 axis
    int llr = (int(pData[2] & 0xff) << 2) | ((pData[3] & 0xc0) >> 6);
    int lud = (int(pData[3] & 0x3f) << 4) | ((pData[4] & 0xf0) >> 4);
    int rlr = (int(pData[4] & 0x0f) << 6) | ((pData[5] & 0xf3) >> 2);
    int rud = (int(pData[5] & 0x03) << 8) | ((pData[6] & 0xff) >> 0);

    // 8 bits-> 10 bits
    int alt = int(pData[7]) << 2;
    int art = int(pData[8]) << 2;

    int btns = (int(pData[10]) << 8) | int(pData[9]);
    int dpad = int(pData[11]);

    if (_pStickCallback)
        _pStickCallback->onStickChanged(llr, lud, rlr, rud, alt, art, dpad, btns);
}
