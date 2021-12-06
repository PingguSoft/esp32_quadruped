#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_
#include "common.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/

//
// MSP VERSION
//
#define VER_MSP_MASTER_MASK  0x00
#define VER_MSP_SLAVE_MASK   0x10
#define VER_MSP_VERSION_MASK 0x0F
//
#define VER_MSP             0x01
#define VER_EXT             0x02
#define VER_MSP_MASTER      (VER_MSP_MASTER_MASK | VER_MSP)
#define VER_EXT_MASTER      (VER_MSP_MASTER_MASK | VER_EXT)
#define VER_MSP_SLAVE       (VER_MSP_SLAVE_MASK | VER_MSP)
#define VER_EXT_SLAVE       (VER_MSP_SLAVE_MASK | VER_EXT)

//
// MULTIMEDIA COMMANDS
//
#define CMD_ACK_REQ         0x80
// camera video
#define CMD_CAM_INFO        (CMD_ACK_REQ | 0x01)
#define CMD_CAM_START       (CMD_ACK_REQ | 0x02)
#define CMD_CAM_STOP        (CMD_ACK_REQ | 0x03)
#define CMD_CAM_REQ_IFRAME  0x04
#define CMD_CAM_SETTINGS    (CMD_ACK_REQ | 0x05)
// video, audio data
#define CMD_VIDEO           0x20
#define CMD_AUDIO           0x21

//
// AUX ASSIGNMENT
//
#define AUX_LIGHT1          0
#define AUX_LIGHT2          1
#define AUX_LIGHT3          2
#define AUX_LIGHT4          3
#define AUX_TRANSFORM       4
#define AUX_FIRE            5
#define AUX_GIMBAL_LOCK     6
#define AUX_GIMBAL_ADJ      7

//
// CAPABILITY FLAG
//
#define CAP_VIDEO           BV(31)
#define CAP_AUDIO           BV(30)


/*
*****************************************************************************************
* MACROS & STRUCTURES
*****************************************************************************************
*/
typedef enum {
    // MSP
    MSP_IDENT               = 100,
    MSP_STATUS              = 101,
    MSP_ATTITUDE            = 108,
    MSP_ALTITUDE            = 109,
    MSP_ANALOG              = 110,
    MSP_MISC                = 114,
    MSP_SET_RAW_RC_TINY     = 150,
    MSP_SET_RAW_RC          = 200,
    MSP_SET_MISC            = 207,

    // DEFINED
    MSP_SET_SW_BUTTON       = 51,

    MSP_SET_MOTOR_LED       = 70,
    MSP_SET_GIMBAL_SPEED    = 71,
    MSP_SET_TRANSFORM_SPEED = 72,
    MSP_SET_GIMBAL_HOMING   = 73,
    MSP_GET_GIMBAL_POSITION = 74,

    MSP_SET_ONLINE          = 80,
    MSP_SET_OFFLINE         = 81,
} MSP_T;


struct param_id {
    u8  version;
    u8  type;
    u8  msp_version;
    u32 capability;
} __attribute__((packed));

struct param_status {
    u16 cycles;
    u16 ctrErr;
    u16 maskSensors;
    u32 flags;
    u8  curSet;
} __attribute__((packed));

struct param_att {
    s16 angleX;     // [-1800;1800] (unit: 1/10 degree)
    s16 angleY;     // [-900;900]   (unit: 1/10 degree)
    s16 heading;    // [-180;180]   (unit: degree)
} __attribute__((packed));

struct param_alt {
    u32 cmAltitude; // cm
    u16 speed;      // cm/s
} __attribute__((packed));

struct param_analog {
    u8  deciVolt;
    u16 sumPowerMeter;
    s16 rssi;
    u16 amperage;
} __attribute__((packed));

struct param_misc {
    u16 powTrig;
    u16 minThr;
    u16 maxThr;
    u16 minCmdThr;
    u16 fsThr;
    u16 ctrArm;
    u32 lifeTime;
    u16 magDec;
    u8  battScale;
    u8  battWarn1;
    u8  battWarn2;
    u8  battCrit;
} __attribute__((packed));

struct param_rc {
    s16 roll;
    s16 pitch;
    s16 yaw;
    s16 throttle;
    s16 aux[10];
    u8  flag;
} __attribute__((packed));

struct param_rc_short {
    u8 roll;
    u8 pitch;
    u8 yaw;
    u8 throttle;
    u8 switches;
} __attribute__((packed));

struct param_motor_led {
    s16 motor[5];
    u8  leds;
} __attribute__((packed));

struct param_gimbal_speed {
    s16 yaw;
    s16 pitch;
    u8  move;
} __attribute__((packed));

struct param_gimbal_pos {
    s16 yaw;
    s16 pitch;
} __attribute__((packed));


/*
*****************************************************************************************
* Class
*****************************************************************************************
*/
class ProtocolCallback {
    public:
        virtual ~ProtocolCallback() { }
        virtual void onRC(struct param_rc *rc)                      { }
        virtual u32  onCapInfo(void)                                { return 0; }
        virtual void onAttitude(struct param_att *att)              { memset(att, 0, sizeof(struct param_att)); }
        virtual void onAltitude(struct param_alt *alt)              { memset(alt, 0, sizeof(struct param_alt)); }
        virtual u8   onBattDV(void)                                 { return 0; }
        virtual s8   onOthers(u8 cmd, u8 *pData, u8 size, u8 *pRes) { return -1; }
};

class ProtocolDevice {
    public:
        virtual ~ProtocolDevice() { }
        virtual void write(u8 *pBuf, u8 size) = 0;
        virtual int  read(void) = 0;
        virtual int  available(void) = 0;
};

class Protocol {
    public:
        Protocol(u8 m_wPacketSize);
        ~Protocol();

        void setDevice(ProtocolDevice *pInterface);
        void setCallback(ProtocolCallback *pCallback);
        void processRx(void);
        void setSwButtonCmd(u8 cmd = MSP_SET_SW_BUTTON)             { m_ucSwBtnCmd = cmd; }

        void send(bool resp, u8 cmd, u8 *pData, u8 size);
        void send(bool resp, ProtocolDevice *pDev, u8 cmd, u8 *pData, u8 size);

    private:
        void _onCommand(u8 cmd, u8 *pData, u8 size, u8 *pRes);

        typedef enum {
            STATE_IDLE,
            STATE_HEADER_START,
            STATE_HEADER_M,
            STATE_HEADER_ARROW,
            STATE_HEADER_SIZE,
            STATE_HEADER_CMD
        } STATE_T;

        u8           *m_pRxPackets;
        u8           *m_pResultPackets;
        u8           m_ucPacketSize;

        STATE_T      m_stateRx;
        u8           m_ucOffset;
        u8           m_ucDataSize;
        u8           m_ucCheckSum;
        u8           m_ucCmd;
        u8           m_ucSwBtnCmd;
        u8           m_ucSwBtnVal;
        ProtocolCallback  *m_pCallback;
        ProtocolDevice    *m_pDevice;
};

#endif

