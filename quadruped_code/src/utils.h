#ifndef _UTILS_H_
#define _UTILS_H_

#include "config.h"

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
#define ARRAY_SIZE(x)                   (sizeof(x) / sizeof((x)[0]))
#define IS_ELAPSED(ts, last, duration)  ((ts) - (last) > (duration))

#define LOG(...)                        printf(__VA_ARGS__)             //Utils.log(__VA_ARGS__)
#define DUMP(name, data, cnt)           Utils::dump(name, data, cnt)

class Utils {
public:
    static void  dump(String name, uint8_t* data, uint16_t cnt);
    static void  log(...);
    static float mapf(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif