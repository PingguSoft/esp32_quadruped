#include <Arduino.h>
#include "StatusLed.h"


/*
*****************************************************************************************
* VARIABLES
*****************************************************************************************
*/

/*
*****************************************************************************************
* Functions
*****************************************************************************************
*/
void StatusLed::setup(void) {
    digitalLeds_initDriver();
    digitalLeds_addStrands((strand_t **)_pStrands, kCtrStrands);
    digitalLeds_resetPixels((strand_t **)_pStrands, kCtrStrands);
}

void StatusLed::setBrightness(uint8_t brightness) {
    _ledStrip.brightLimit = brightness;
}

void StatusLed::set(int pos, color_idx_t idx, uint16_t msBlink, color_idx_t offColor, bool update) {
    set(pos, getColor(idx), msBlink, getColor(offColor), update);
}

void StatusLed::set(int pos, uint8_t r, uint8_t g, uint8_t b, uint16_t msBlink, pixelColor_t offColor, bool update) {
    strand_t     *pStrand = (strand_t *)_pStrands[0];
    pixelColor_t color    = pixelFromRGB(r * pStrand->brightLimit / 255, g * pStrand->brightLimit / 255,
                                            b * pStrand->brightLimit / 255);

    set(pos, color, msBlink, offColor, update);
}

void StatusLed::set(int pos, pixelColor_t color, uint16_t msBlink, pixelColor_t offColor, bool update) {
    strand_t     *pStrand = (strand_t *)_pStrands[0];
    pixelColor_t c        = color;

    c.r = c.r * pStrand->brightLimit / 255;
    c.g = c.g * pStrand->brightLimit / 255;
    c.b = c.b * pStrand->brightLimit / 255;
    if (pos < pStrand->numPixels) {
        pStrand->pixels[pos] = c;
        _isAllSame = false;
    } else {
        for (uint16_t i = 0; i < pStrand->numPixels; i++) {
            pStrand->pixels[i] = c;
        }
        _isAllSame = true;
        pos = 0;
    }

    if (msBlink > 0) {
        _lLastBlinkTS[pos]  = millis();
        _colorBlinkOn[pos]  = c;
        _colorBlinkOff[pos] = offColor;
    }
    _wBlinkPeriod[pos] = msBlink;

    if (update) {
        digitalLeds_drawPixels((strand_t **)_pStrands, kCtrStrands);
    }
}

void StatusLed::update() {
    digitalLeds_drawPixels((strand_t **)_pStrands, kCtrStrands);
}

void StatusLed::loop(unsigned long ts) {
    strand_t *pStrand  = (strand_t *)_pStrands[0];
    bool      isUpdate = false;

    for (int i = 0; i < _ledStrip.numPixels; i++) {
        if (_wBlinkPeriod[i] > 0) {
            if (ts - _lLastBlinkTS[i] > _wBlinkPeriod[i]) {
                pixelColor_t c = (pStrand->pixels[i].num == _colorBlinkOff[i].num) ? _colorBlinkOn[i] : _colorBlinkOff[i];
                _lLastBlinkTS[i]   = ts;
                isUpdate = true;

                if (_isAllSame) {
                    for (int j = 0; j < _ledStrip.numPixels; j++) {
                        pStrand->pixels[j] = c;
                    }
                    break;
                } else {
                    pStrand->pixels[i] = c;
                }
            }
        }
    }

    if (isUpdate) {
        update();
    }
}
