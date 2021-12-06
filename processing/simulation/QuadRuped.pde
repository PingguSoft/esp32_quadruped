final color kCOLOR_BONE  = color(40, 200, 110);
final color kCOLOR_JOINT = color(255, 255, 255);
final color kCOLOR_BODY  = color(50, 150, 220);
final color kCOLOR_HEAD  = color(255, 0, 0);
final float kLEG_WIDTH   = 5.0f;


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

class Leg {
    float _coxaLength;
    float _femurLength;
    float _tibiaLength;
    float _coxaOffsetZ;

    Vector _vOffFromCenter = new Vector();
    Vector _vInitPos       = new Vector();
    Vector _vPos           = new Vector();
    Vector _vTipOffset     = new Vector(-30, 0, 0);

    float _angleCoxa;
    float _angleFemur;
    float _angleTibia;

    int     _pos;
    boolean _isDebug;

    float fixAngle(float angle) {
        if (angle < -180) {
            angle += 360;
        } else if (angle > 180) {
            angle -= 360;
        }
        return angle;
    }

    float roundUp(float v) {
        return round(v * 100.0f) / 100.0f;
    }

    void enableDebug(boolean en) { 
        _isDebug = en;
    }

    boolean isDebugEnabled() {
        return _isDebug;
    }

    Leg(int pos, float bodyWidth, float bodyHeight, float coxaLength, float coxaOffsetZ, float femurLength, float tibiaLength) {
        _pos         = pos;
        _isDebug     = false;
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
        signY   = (pos < 2) ? 1 : -1;
        signX   = (_pos == 0 || _pos == 3) ? 1 : -1;

        _vOffFromCenter.x = signX * offsetX;
        _vOffFromCenter.y = signY * offsetY;
        _vOffFromCenter.z = tibiaLength;

        _vInitPos.x = signX * ((_tibiaLength * sin(radians(45))) - (_femurLength * sin(radians(45))));
        _vInitPos.y = signY * coxaLength;
        _vInitPos.z = coxaOffsetZ + roundUp(sqrt(sq(_femurLength) + sq(_tibiaLength)));    // angle between femur and tibia = 90 degree

        _vOffFromCenter.z += _vInitPos.z;

        _vPos.set(_vInitPos);
        if (true) { //_isDebug) {
            println(String.format("init   leg:%d, off from center (%6.1f, %6.1f)", pos, 
                _vOffFromCenter.x, _vOffFromCenter.y));
            println(String.format("init   leg:%d, %6.1f, %6.1f, %6.1f", pos, 
                _vPos.x, _vPos.y, _vPos.z));
        }
    }

    Vector bodyIK(Vector vMov, Rotator rRot) {
        float mx = vMov.x + _vTipOffset.x;
        float my = (_pos < 2) ? (vMov.y + _vTipOffset.y) : -(vMov.y + _vTipOffset.y);
        //float my = vMov.y + _vTipOffset.y;
        
        // body ik
        float totX = _vInitPos.x + _vOffFromCenter.x + mx;
        float totY = _vInitPos.y + _vOffFromCenter.y + my;

        // yaw
        float alpha_0 = atan2(totX, totY);
        float alpha_1 = alpha_0 + radians(rRot.yaw);

        float dist   = sqrt(sq(totX) + sq(totY));
        float bodyIKX = roundUp(sin(alpha_1) * dist - totX);
        float bodyIKY = roundUp(cos(alpha_1) * dist - totY);

        // pitch, roll
        float pitchZ = tan(radians(rRot.pitch)) * totX;
        float rollZ  = tan(radians(rRot.roll)) * totY;
        float bodyIKZ = roundUp(rollZ + pitchZ);
        
        _vPos.set(_vInitPos.x + mx + bodyIKX, _vInitPos.y + my + bodyIKY, _vInitPos.z + vMov.z - bodyIKZ);

        if (_isDebug) {
            println(String.format("newpos leg:%d, %6.1f, %6.1f, %6.1f [%6.1f, %6.1f]", _pos, _vPos.x, _vPos.y, _vPos.z,
                    bodyIKX, bodyIKY));
        }

        return _vPos;
    }

    void legIK(Vector tgt) {
        float h1 = sqrt(sq(_coxaLength) + sq(_coxaOffsetZ));
        float h2 = sqrt(sq(tgt.z) + sq(tgt.y));
        float a0 = atan(abs(tgt.y) / tgt.z);
        float a1 = atan(_coxaLength / _coxaOffsetZ);
        float a2 = atan(_coxaOffsetZ / _coxaLength);
        float a3 = asin(h1 * sin(a2 + radians(90)) / h2);
        float a4 = radians(180) - (a3 + a2 + radians(90));
        float a5 = a1 - a4;
        float th = a0 - a5;

        float r0 = h1 * sin(a4) / sin(a3);
        float h  = sqrt(sq(r0) + sq(abs(tgt.x)));
        
        float phi= asin(tgt.x / h);
        float ts = acos((sq(h) + sq(_femurLength) - sq(_tibiaLength)) / (2 * h * _femurLength)) - phi;
        float tw = acos((sq(_tibiaLength) + sq(_femurLength) - sq(h)) / (2 * _femurLength * _tibiaLength));

        if (Float.isNaN(th) || Float.isNaN(ts) || Float.isNaN(tw)) {
            println(String.format("ERROR  leg:%d, %6.1f, %6.1f, %6.1f\n", _pos, th, ts, tw));            
            return;
        }

        float ac = degrees(th);
        float af = 90 - degrees(ts);
        float at = degrees(tw);

        _angleCoxa  = fixAngle(ac);    // zero base
        _angleFemur = fixAngle(af);    // zero base 0 - 180
        _angleTibia = fixAngle(at);    // zero base 0 - 180

        if (_isDebug) {
            println(String.format("angle  leg:%d, %6.1f, %6.1f, %6.1f\n", _pos, _angleCoxa, _angleFemur, _angleTibia));
        }
    }

    void move(Vector vMov, Rotator rRot) {
        Vector tgt = bodyIK(vMov, rRot);
        legIK(tgt);
    }

    private void jointX(float x, float y, float z, float a, color c) {
        translate(x, y, z);
        rotateX(radians(a));
        fill(c);
        sphere(kLEG_WIDTH); //> draws a sphere
    }

    private void jointZ(float x, float y, float z, float a, color c) {
        translate(x, y, z);
        rotateZ(radians(a));
        fill(c);
        sphere(kLEG_WIDTH);
    }

    private void jointY(float x, float y, float z, float a, color c) {
        translate(x, y, z);
        rotateY(radians(a));
        fill(c);
        sphere(kLEG_WIDTH);
    }

    void draw() {
        pushMatrix();

        //
        // y is inverted in the screen '-'
        //

        // hip
        int sign = (_pos < 2) ? 1 : -1;
        jointX(_vOffFromCenter.x, _vOffFromCenter.y, _vOffFromCenter.z, _angleCoxa, 
            (_pos == 0) ? kCOLOR_HEAD : kCOLOR_JOINT);
        translate(0, sign * _coxaLength / 2, 0);
        fill(kCOLOR_BONE);
        box(kLEG_WIDTH, _coxaLength, kLEG_WIDTH);

        // femur
        jointY(0, sign * _coxaLength / 2, 0, - 90 - _angleFemur, kCOLOR_JOINT);
        translate(0, 0, _femurLength / 2);
        fill(kCOLOR_BONE);
        box(kLEG_WIDTH, kLEG_WIDTH, _femurLength);

        // tibia
        jointY(0, 0, _femurLength / 2, _angleTibia - 180, kCOLOR_JOINT);
        translate(0, 0, _tibiaLength / 2);
        fill(kCOLOR_BONE);
        box(kLEG_WIDTH, kLEG_WIDTH, _tibiaLength);

        jointY(0, 0, _tibiaLength / 2, 0, kCOLOR_JOINT);

        popMatrix();
    }
}

boolean IS_FRONT_LEG(int leg) {
    return (leg == 0 || leg == 3);    
}

boolean IS_RIGHT_LEG(int leg) {
    return (leg < 2);
}

class QuadRuped {
    Leg       _legs[];
    Vector   _vMov;
    Rotator  _rRot;    // x-> pitch, y->roll, z->yaw
    Gait      _gait;
    boolean   _isWalk;
    int       _debugLegMask = 0x1;

    QuadRuped(float bodyWidth, float bodyHeight, float coxaLen, float coxaOffsetZ, float femurLen, float tibiaLen) {
        _legs       = new Leg[4];
        _vMov       = new Vector(0, 0, 0);
        _rRot       = new Rotator(0, 0, 0);

        int mask;
        for (int i = 0; i < _legs.length; i++) {
            mask = 1 << i;
            _legs[i] = new Leg(i, bodyWidth, bodyHeight, coxaLen, coxaOffsetZ, femurLen, tibiaLen);
            _legs[i].enableDebug((_debugLegMask & mask) == mask);
            _legs[i].move(_vMov, _rRot);
        }
        _gait = new GaitTrot(60);
        println("-----------------------");
    }

    void draw() {
        // body
        pushMatrix();
        translate(0, 0, _vMov.z);
        rotateY(radians(_rRot.pitch));
        rotateX(radians(-_rRot.roll));
        if (!_isWalk) {
            rotateZ(radians(_rRot.yaw));
        }
        strokeWeight(15);
        stroke(kCOLOR_BODY);
        noFill();
        beginShape();
        for (int i = 0; i <= _legs.length; i++) {
            vertex(_legs[i % _legs.length]._vOffFromCenter.x, _legs[i % _legs.length]._vOffFromCenter.y, 
                _legs[i % _legs.length]._vOffFromCenter.z);
        }
        endShape();
        noStroke();

        // legs
        for (int i = 0; i < _legs.length; i++) {
            _legs[i].draw();
        }
        popMatrix();
    }

    Vector getPos() {
        return _vMov;
    }

    Rotator getRot() {
        return _rRot;
    }

    void update(boolean isWalk) {
        //_vMov.dump("_vMov");
        //_rRot.dump("_rRot");

        _isWalk = isWalk;
        if (isWalk) {
            int    sign;
            float  yaw = _rRot.yaw;

            for (int i = 0; i < _legs.length; i++) {
                sign = IS_RIGHT_LEG(i) ? 1 : -1;
                
                Vector dir = new Vector(
                    Gait.kPRECISION + (_vMov.x + ((yaw / 8)) * -sign), 
                    Gait.kPRECISION + (_vMov.y + ((yaw / 8)) *  sign), 
                    _vMov.z);

                if (_legs[i].isDebugEnabled()) {
                    println(String.format("dir    leg:%d, %6.1f, %6.1f, %6.1f", i, dir.x, dir.y, dir.z));
                }
                
                Rotator rot = new Rotator(_rRot);
                _gait.doStep(i, dir, rot);
                
                if (_legs[i].isDebugEnabled()) {
                    println(String.format("movpos leg:%d, (%6.1f, %6.1f, %6.1f), (P:%6.1f, R:%6.1f)", i, dir.x, dir.y, dir.z, rot.pitch, rot.roll));
                }
                _legs[i].move(dir, rot);
                _rRot.set(rot);
            }

            _rRot.yaw = yaw;
        } else {
            for (int i = 0; i < _legs.length; i++) {
                _legs[i].move(_vMov, _rRot);
            }
        }
        //println("-----------------------");
    }
}
