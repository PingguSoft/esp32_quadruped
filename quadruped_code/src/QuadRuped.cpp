#include "QuadRuped.h"

/*
*****************************************************************************************
* CONSTANTS
*****************************************************************************************
*/

/*
    Z      TOP VIEW
               | -Y
               |
          ---------
          | 2     3 |               H
-X -------|    +    |-------- +X    E
          | 1     0 |               A
          ---------                 D
               |
               |
               | +Y
*/

/*
*****************************************************************************************
* MACROS
*****************************************************************************************
*/

/*
*****************************************************************************************
* LEG Class
*****************************************************************************************
*/
Vector Leg::_vTipOffset(-20, 0, 0);

float Leg::fixAngle(float angle) {
    if (angle < -180) {
        angle += 360;
    } else if (angle >= 180) {
        angle -= 360;
    }
    return angle;
}

float Leg::roundUp(float v) {
    return round(v * 100.0f) / 100.0f;
}

void Leg::set(int pos, float bodyWidth, float bodyHeight, float coxaLength, float coxaOffsetZ, float femurLength, float tibiaLength) {
    _pos = pos;
    _coxaLength  = coxaLength;
    _femurLength = femurLength;
    _tibiaLength = tibiaLength;
    _coxaOffsetZ = coxaOffsetZ;

    int   signY;
    int   signX;
    float offsetX;
    float offsetY;

    offsetX = bodyHeight / 2;
    offsetY = bodyWidth  / 2;
    signY   = IS_RIGHT_LEG(pos) ? 1 : -1;
    signX   = IS_FRONT_LEG(pos) ? 1 : -1;

    _vOffFromCenter.x = signX * offsetX;
    _vOffFromCenter.y = signY * offsetY;
    _vOffFromCenter.z = tibiaLength;

    _vInitPos.x = signX * ((_tibiaLength * sin(radians(45))) - (_femurLength * sin(radians(45))));
    _vInitPos.y = signY * coxaLength;
    _vInitPos.z = coxaOffsetZ + roundUp(sqrt(sq(_femurLength) + sq(_tibiaLength)));    // angle between femur and tibia = 90 degree

    _vOffFromCenter.z += _vInitPos.z;
    if (_isDebug) {
        LOG("init   leg:%d, off from center (%6.1f, %6.1f)\n", pos,
            _vOffFromCenter.x, _vOffFromCenter.y);
        LOG("init   leg:%d, %6.1f, %6.1f, %6.1f\n", pos,
            _vInitPos.x, _vInitPos.y, _vInitPos.z);
    }
}

void Leg::bodyIK(Vector *pMov, Rotator *pRot, Vector *pTgt) {
    float mx = pMov->x + _vTipOffset.x;
    float my = pMov->y + _vTipOffset.y;

    // body ik
    float totX = _vInitPos.x + _vOffFromCenter.x + mx;
    float totY = _vInitPos.y + _vOffFromCenter.y + my;

    // yaw
    float alpha_0 = atan2(totX, totY);
    float alpha_1 = alpha_0 + radians(pRot->yaw);

    float dist   = sqrt(sq(totX) + sq(totY));
    float bodyIKX = roundUp(sin(alpha_1) * dist - totX);
    float bodyIKY = roundUp(cos(alpha_1) * dist - totY);

    // pitch, roll
    float pitchZ = tan(radians(pRot->pitch)) * totX;
    float rollZ  = tan(radians(pRot->roll)) * totY;
    float bodyIKZ = roundUp(rollZ + pitchZ);

    pTgt->set(_vInitPos.x + mx + bodyIKX, _vInitPos.y + my + bodyIKY, _vInitPos.z + pMov->z - bodyIKZ);
    if (_isDebug) {
        LOG("newpos leg:%d, %6.1f, %6.1f, %6.1f\n", _pos, pTgt->x, pTgt->y, pTgt->z);
    }
}

//
// https://www.adham-e.dev/pdf/IK_Model.pdf
//
void Leg::legIK(Vector *pTgt, JointAngle *pJA) {
    float h1 = sqrt(sq(_coxaLength) + sq(_coxaOffsetZ));
    float h2 = sqrt(sq(pTgt->z) + sq(pTgt->y));
    float a0 = atan(abs(pTgt->y) / pTgt->z);
    float a1 = atan(_coxaLength / _coxaOffsetZ);
    float a2 = atan(_coxaOffsetZ / _coxaLength);
    float a3 = asin(h1 * sin(a2 + radians(90)) / h2);
    float a4 = radians(180) - (a3 + a2 + radians(90));
    float a5 = a1 - a4;
    float th = a0 - a5;

    float r0 = h1 * sin(a4) / sin(a3);
    float h  = sqrt(sq(r0) + sq(abs(pTgt->x)));

    float phi= asin(pTgt->x / h);
    float ts = acos((sq(h) + sq(_femurLength) - sq(_tibiaLength)) / (2 * h * _femurLength)) - phi;
    float tw = acos((sq(_tibiaLength) + sq(_femurLength) - sq(h)) / (2 * _femurLength * _tibiaLength));

    if (isnan(th) || isnan(ts) || isnan(tw)) {
        LOG("ERROR  leg:%d, %6.1f, %6.1f, %6.1f\n", _pos, th, ts, tw);
        return;
    }

    float ac = degrees(th);             // -90 ~ 90
    float af = -degrees(ts);            // -90 ~ 90
    float at = degrees(tw) - 90;        // -90 ~ 90

#if (CONFIG_BODY == CONFIG_BODY_SPOTMICRO)
    ac = IS_RIGHT_LEG(_pos) ? ac : -ac;
#elif (CONFIG_BODY == CONFIG_BODY_KANGAL)   // coxa rotation reversed
    ac = IS_RIGHT_LEG(_pos) ? -ac : ac;
#endif
    // zero base
    pJA->set(fixAngle(ac), fixAngle(af), fixAngle(at));
    if (_isDebug) {
        LOG("angle  leg:%d, %6.1f, %6.1f, %6.1f\n", _pos, pJA->getCoxa(), pJA->getFemur(), pJA->getTibia());
    }
}

void Leg::move(Vector *pMov, Rotator *pRot) {
    Vector tgt;

    bodyIK(pMov, pRot, &tgt);
    legIK(&tgt, &_jointAngle);
}


/*
*****************************************************************************************
* QuadRuped Class
*****************************************************************************************
*/
QuadRuped::QuadRuped(float bodyWidth, float bodyHeight, float coxaLen, float coxaOffsetZ, float femurLen, float tibiaLen) {
    _debugLegMask = 0x00;
    _lLastChcked  = 0;
    _iInterval    = 1000 / 50;              // default update interval (50Hz)

    Vector  v;                              // default vector  (0, 0, 0)
    Rotator r;                              // default rotator (0, 0, 0)

    for (int i = 0; i < BODY_NUM_LEGS; i++) {
        _legs[i].enableDebug(_debugLegMask & _BV(i));
        _legs[i].set(i, bodyWidth, bodyHeight, coxaLen, coxaOffsetZ, femurLen, tibiaLen);
        _legs[i].move(&v, &r);
    }
    LOG("-----------------------\n");
}

void QuadRuped::setup(Hardware *pHW) {
    _pHW = pHW;
    _iInterval = 1000 / _pHW->getServoFreq();
}

void QuadRuped::setDebugMask(int mask) {
    _debugLegMask = mask;
    for (int i = 0; i < BODY_NUM_LEGS; i++) {
        _legs[i].enableDebug(_debugLegMask & _BV(i));
    }
}

void QuadRuped::update(unsigned long ts, boolean isWalk, Vector *pMov, Rotator *pRot) {
    if (_lLastChcked != 0 && (ts - _lLastChcked) < _iInterval)
        return;

    if (isWalk) {
        Vector  move;
        Rotator rot;
        float   yaw = pRot->yaw;

        rot.set(0, pRot->pitch, pRot->roll);  // clear yaw while moving
        for (int i = 0; i < BODY_NUM_LEGS; i++) {
            int sign = IS_RIGHT_LEG(i) ? 1 : -1;
            if (pMov->x < 0) {
                sign = -sign;
            }
            move.set(PRECISION + pMov->x + ((yaw / 8) * -sign),
                     PRECISION + pMov->y + ((yaw / 8) *  sign),
                     pMov->z);

            _pGait->doStep(i, &move, &rot, int(_debugLegMask & _BV(i)));

            _legs[i].move(&move, &rot);
            _pHW->setLeg(i, _legs[i].getJointPtr()->getCoxa() * 10.0f, _legs[i].getJointPtr()->getFemur() * 10.0f, _legs[i].getJointPtr()->getTibia() * 10.0f);
        }
    } else {
        for (int i = 0; i < BODY_NUM_LEGS; i++) {
            _legs[i].move(pMov, pRot);
            _pHW->setLeg(i, _legs[i].getJointPtr()->getCoxa() * 10.0f, _legs[i].getJointPtr()->getFemur() * 10.0f, _legs[i].getJointPtr()->getTibia() * 10.0f);
        }
    }

    if (_debugLegMask) {
        LOG("-----------------------\n");
    }

    _lLastChcked = ts;
}
