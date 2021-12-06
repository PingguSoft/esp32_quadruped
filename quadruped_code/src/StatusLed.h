#ifndef _STATUS_LED_H_
#define _STATUS_LED_H_
#include <esp32_digital_led_lib.h>
#include "config.h"
#include "utils.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/
#define HUE(h)                  (65535L * (h) / 360)
#define SAT(s)                  (255L   * (s) / 100)
#define VAL(v)                  (255L   * (v) / 100)

/*
*****************************************************************************************
* CLASS
*****************************************************************************************
*/
class StatusLed {
public:
    const static int ALL_POS = 0xff;

    typedef enum {
        COLOR_BLACK = 0,
        COLOR_PINK,
        COLOR_PURPLE,
        COLOR_BLUE,
        COLOR_LIGHTBLUE,
        COLOR_CYAN,
        COLOR_GREEN,
        COLOR_YELLOW,
        COLOR_ORANGE,
        COLOR_RED,
        COLOR_WHITE
    } color_idx_t;
    
    StatusLed() : 
        _ledStrip   { 0, PIN_LED_STRIP, LED_WS2812B_V2, 64, 2 },
        _pStrands { &_ledStrip },        
        _tblColor   {
            pixelFromRGB(0, 0, 0),          // black
            pixelFromRGB(255, 192, 203),    // pink
            pixelFromRGB(128, 0, 128),      // purple
            pixelFromRGB(0, 0, 255),        // blue
            pixelFromRGB(173, 216, 230),    // lightblue
            pixelFromRGB(0, 255, 255),      // cyan
            pixelFromRGB(0, 255, 0),        // green
            pixelFromRGB(255, 255, 0),      // yellow
            pixelFromRGB(255, 165, 0),      // orange
            pixelFromRGB(255, 0, 0),        // red
            pixelFromRGB(255, 255, 255)     // white
        }
    {
    }

    pixelColor_t getColor(uint8_t idx) {
        if (idx < ARRAY_SIZE(_tblColor))
            return _tblColor[idx];
        return _tblColor[0];
    }
    
    void setup(void);
    void setBrightness(uint8_t brightness);
    void set(int pos, color_idx_t idx, uint16_t msBlink = 0, color_idx_t offColor = StatusLed::COLOR_BLACK, bool update = true);
    void set(int pos, uint8_t r, uint8_t g, uint8_t b, uint16_t msBlink = 0, pixelColor_t offColor = pixelFromRGB(0, 0, 0), bool update = true);
    void set(int pos, pixelColor_t color, uint16_t msBlink = 0, pixelColor_t offColor = pixelFromRGB(0, 0, 0), bool update = true);
    void update();
    void loop(unsigned long ts);

private:
    static const int kCtrStrands   = 1;

    strand_t        _ledStrip;
    strand_t       *_pStrands[kCtrStrands];
    pixelColor_t    _tblColor[11];
    unsigned long   _lLastBlinkTS[2];
    uint16_t        _wBlinkPeriod[2];
    pixelColor_t    _colorBlinkOn[2];
    pixelColor_t    _colorBlinkOff[2];
    bool            _isAllSame;
};

#endif