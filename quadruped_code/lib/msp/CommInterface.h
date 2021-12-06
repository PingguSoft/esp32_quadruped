/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 see <http://www.gnu.org/licenses/>
*/


#ifndef _COMM_INTERFACE_H_
#define _COMM_INTERFACE_H_

#include <stdarg.h>
#include <BluetoothSerial.h>
#include "common.h"
#include "Protocol.h"

class CommInterface {

public:
    CommInterface(u8 size)          { m_pProtocol = new Protocol(size); m_isConnected = FALSE; }

    virtual ~CommInterface()        { if (m_pProtocol) delete m_pProtocol;   }

    void setConnected(bool conn)    { m_isConnected = conn; }

    bool isConnected()              { return m_isConnected; }

    Protocol *getProtocol()         { return m_pProtocol;   }

    virtual int loop(void) = 0;

private:
    Protocol     *m_pProtocol;
    bool          m_isConnected;
};


class CommSerial : public CommInterface {

public:
    CommSerial(int arg);

    ~CommSerial();

    virtual int loop(void);
};


class CommSerialBT : public CommInterface {

public:
    CommSerialBT(int arg);

    CommSerialBT(char *name, int bufSize);

    ~CommSerialBT();

    virtual int loop(void);
};


#endif

