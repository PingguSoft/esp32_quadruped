#include "common.h"
#include "CommInterface.h"

class SerialDev : public ProtocolDevice {
    public:
        SerialDev() {
        }


        SerialDev(CommSerial* x):m_pParent(x) {
        }

        virtual ~SerialDev() {
        }

        virtual void write(u8 *pBuf, u8 size) {
            Serial.write(pBuf, size);
        }

        virtual int read(void) {
            return Serial.read();
        }

        virtual int available(void) {
            return Serial.available();
        }

    private:
        CommSerial  *m_pParent;
};

int CommSerial::loop() {
    getProtocol()->processRx();
    return 0;
}

CommSerial::CommSerial(int arg) : CommInterface(arg) {
    getProtocol()->setDevice(new SerialDev(this));
    setConnected(TRUE);
}

CommSerial::~CommSerial() {
}

