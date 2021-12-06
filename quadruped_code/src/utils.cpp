#include "utils.h"

void Utils::dump(String name, uint8_t* data, uint16_t cnt) {
    uint8_t  i;
    uint8_t  b;
    uint16_t addr = 0;

    LOG(PSTR("-- %s buf size : %d -- \n"), name.c_str(), cnt);
    while (cnt) {
        LOG(PSTR("%08x - "), addr);

        for (i = 0; (i < 16) && (i < cnt); i++) {
            b = *(data + i);
            LOG(PSTR("%02x "), b);
        }

        LOG(PSTR(" : "));
        for (i = 0; (i < 16) && (i < cnt); i++) {
            b = *(data + i);
            if ((b > 0x1f) && (b < 0x7f))
                LOG(PSTR("%c"), b);
            else
                LOG(PSTR("."));
        }
        LOG(PSTR("\n"));
        data += i;
        addr += i;
        cnt -= i;
    }
}

void Utils::log(...) {
}

float Utils::mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    const float dividend = out_max - out_min;
    const float divisor = in_max - in_min;
    const float delta = x - in_min;

    return (delta * dividend) / divisor + out_min;
}
