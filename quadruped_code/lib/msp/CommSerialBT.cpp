#include "CommInterface.h"

#define LOG(...)    printf(__VA_ARGS__)

class SerialBTDev : public ProtocolDevice {
    public:
        void init(char *name) {
            m_pSerialBT = new BluetoothSerial();
            m_pSerialBT->register_callback([](esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
                if (event == ESP_SPP_SRV_OPEN_EVT) {
                    LOG("BT:CONNECTED\n");
                } else if (event == ESP_SPP_CLOSE_EVT) {
                    LOG("BT:CLOSED\n");
                }
            });
            bool ret = m_pSerialBT->begin(name);
            if (!ret) {
                LOG("bluetooth begin failure\n");
            }
        }
        
        SerialBTDev() {
            init((char*)"ESP32-SPP");
        }

        SerialBTDev(char *name) {
            init(name);
        }

        SerialBTDev(CommSerialBT* x):m_pParent(x) {
            init((char*)"ESP32-SPP");
        }

        SerialBTDev(char *name, CommSerialBT* x):m_pParent(x) {
            init(name);
        }

        virtual ~SerialBTDev() {
        }

        virtual void write(u8 *pBuf, u8 size) {
            if (m_pSerialBT)
                m_pSerialBT->write(pBuf, size);
        }

        virtual int read(void) {
            return (m_pSerialBT ? m_pSerialBT->read() : -1);
        }

        virtual int available(void) {
            return (m_pSerialBT ? m_pSerialBT->available() : 0);
        }

    private:
        CommSerialBT    *m_pParent;
        BluetoothSerial *m_pSerialBT;
};

int CommSerialBT::loop() {
    getProtocol()->processRx();
    return 0;
}

CommSerialBT::CommSerialBT(int bufSize) : CommInterface(bufSize) {
    getProtocol()->setDevice(new SerialBTDev(this));
    setConnected(TRUE);
}

CommSerialBT::CommSerialBT(char *name, int bufSize) : CommInterface(bufSize) {
    getProtocol()->setDevice(new SerialBTDev(name, this));
    setConnected(TRUE);
}

CommSerialBT::~CommSerialBT() {
}
