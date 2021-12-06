#include "Protocol.h"

Protocol::Protocol(u8 ucPacketSize) {
    m_stateRx        = STATE_IDLE;
    m_ucOffset       = 0;
    m_ucDataSize     = 0;
    m_ucCheckSum     = 0;
    m_ucPacketSize   = ucPacketSize;
    m_pRxPackets     = new u8[m_ucPacketSize];
    m_pResultPackets = new u8[m_ucPacketSize];
    m_ucSwBtnCmd     = 0;
}

Protocol::~Protocol() {
    delete m_pRxPackets;
    delete m_pResultPackets;
    delete m_pDevice;
}

void Protocol::setDevice(ProtocolDevice *pInterface) {
    m_pDevice = pInterface;
}

void Protocol::setCallback(ProtocolCallback *pCallback) {
    m_pCallback  = pCallback;
}

void Protocol::send(bool resp, u8 cmd, u8 *pData, u8 size) {
    send(resp, m_pDevice, cmd, pData, size);
}

void Protocol::send(bool resp, ProtocolDevice *pDev, u8 cmd, u8 *pData, u8 size) {
    u8  header[5];
    u8  checkSum = 0;

    header[0] = '$';
    header[1] = 'M';
    header[2] = resp ? '>' : '<';
    header[3] = size;
    header[4] = cmd;

    for (u8 i = 3; i < sizeof(header); i++) {
        checkSum  ^= header[i];
    }

    for (u8 i = 0; i < size; i++) {
        u8 b = pData[i];
        checkSum = checkSum ^ b;
    }

    if (pDev) {
        pDev->write(header, sizeof(header));
        pDev->write(pData, size);
        pDev->write(&checkSum, sizeof(checkSum));
    }
}

void Protocol::_onCommand(u8 cmd, u8 *pData, u8 size, u8 *pRes) {
    s8  ret = -1;

    switch (cmd) {
        case MSP_IDENT: {
            struct param_id *id = (struct param_id *)pRes;

            ret = sizeof(struct param_id);
            id->version = 240;
            id->type    = 3;

            if (m_pCallback)
                id->capability = m_pCallback->onCapInfo();
        }
        break;

        case MSP_STATUS: {
            static u16 ctr = 0;
            struct param_status *stat = (struct param_status *)pRes;

            ret = sizeof(struct param_status);
            memset(stat, 0, ret);
            stat->cycles = ctr++;
        }
        break;

        case MSP_ATTITUDE: {
            struct param_att *att = (struct param_att *)pRes;

            ret = sizeof(struct param_att);

            if (m_pCallback)
                m_pCallback->onAttitude(att);
            else
                memset(att, 0, ret);
        }
        break;

        case MSP_ALTITUDE: {
            struct param_alt *alt = (struct param_alt *)pRes;

            ret = sizeof(struct param_alt);

            if (m_pCallback)
                m_pCallback->onAltitude(alt);
            else
                memset(alt, 0, ret);
        }
        break;

        case MSP_ANALOG: {
            struct param_analog *anal = (struct param_analog *)pRes;

            ret = sizeof(struct param_analog);
            memset(anal, 0, ret);
            if (m_pCallback)
                anal->deciVolt = m_pCallback->onBattDV();
        }
        break;

        case MSP_MISC: {
            struct param_misc *misc = (struct param_misc *)pRes;

            ret = sizeof(struct param_misc);
            memset(misc, 0, ret);
            misc->minThr    = 1000;
            misc->maxThr    = 2000;
            misc->minCmdThr = 1000;
            misc->fsThr     = 1200;
        }
        break;

        case MSP_SET_RAW_RC: {
            struct param_rc *rc = (struct param_rc *)pData;

            rc->flag = 0;
            if (m_ucSwBtnCmd > 0) {
                for (u8 i = 0; i < 6; i++) {
                    rc->aux[4 + i] = (m_ucSwBtnVal & BV(i)) ? 2000 : 1000;
                }
            }
            if (m_pCallback)
                m_pCallback->onRC(rc);
        }
        break;

        case MSP_SET_RAW_RC_TINY: {
            struct param_rc_short *rc = (struct param_rc_short *)pData;
            struct param_rc rcl;

            if (m_pCallback) {
                rcl.throttle = 1000 + rc->throttle * 4;
                rcl.yaw      = 1000 + rc->yaw * 4;
                rcl.roll     = 1000 + rc->roll * 4;
                rcl.pitch    = 1000 + rc->pitch * 4;

                for (u8 i = 0; i < 8; i++) {
                    rcl.aux[i] = (rc->switches & BV(i)) ? 2000 : 1000;
                }
                m_pCallback->onRC(&rcl);
            }
        }
        break;

        default:
            if (m_ucSwBtnCmd > 0 && cmd == m_ucSwBtnCmd) {
                m_ucSwBtnVal = *pData;
            } else if (m_pCallback) {
                ret = m_pCallback->onOthers(cmd, pData, size, pRes);
            }
            break;
    }

    if (ret >= 0) {
        send(true, m_pDevice, cmd, pRes, ret);
    }
}

void Protocol::processRx(void) {
    if (!m_pDevice)
        return;

    int size = m_pDevice->available();

    for (int i = 0; i < size; i++) {
        u8 ch = m_pDevice->read();
        switch (m_stateRx) {
            case STATE_IDLE:
                if (ch == '$')
                    m_stateRx = STATE_HEADER_START;
                break;

            case STATE_HEADER_START:
                m_stateRx = (ch == 'M') ? STATE_HEADER_M : STATE_IDLE;
                break;

            case STATE_HEADER_M:
                m_stateRx = (ch == '<') ? STATE_HEADER_ARROW : STATE_IDLE;
                break;

            case STATE_HEADER_ARROW:
                if (ch > m_ucPacketSize) { // now we are expecting the payload size
                    m_stateRx = STATE_IDLE;
                    continue;
                }
                m_ucDataSize = ch;
                m_ucCheckSum = ch;
                m_ucOffset   = 0;
                m_stateRx    = STATE_HEADER_SIZE;
                break;

            case STATE_HEADER_SIZE:
                m_ucCmd       = ch;
                m_ucCheckSum ^= ch;
                m_stateRx     = STATE_HEADER_CMD;
                break;

            case STATE_HEADER_CMD:
                if (m_ucOffset < m_ucDataSize) {
                    m_ucCheckSum              ^= ch;
                    m_pRxPackets[m_ucOffset++] = ch;
                } else {
                    if (m_ucCheckSum == ch) {
                        _onCommand(m_ucCmd, m_pRxPackets, m_ucDataSize, m_pResultPackets);
                    }
                    m_stateRx = STATE_IDLE;
                }
                break;
        }
    }
}
