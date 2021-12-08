public abstract class Gait {
    public final static float kUNIT_MM   = 1f;
    public final static float kPRECISION = 0.1f;

    protected float           _fFreq;
    protected GaitParam       _paramLegs[] = new GaitParam[4];
    protected float           _fSwingAmplitude;
    protected int             _iSwingLeg;
    protected boolean         _isComp;

    protected Vector    _vecStep;
    protected float     _stepsPerSec;
    
    boolean IS_FRONT_LEG(int leg) {
        return (leg == 0 || leg == 3);
    }
    
    boolean IS_RIGHT_LEG(int leg) {
        return (leg < 2);
    }
    
    public Gait(float fSwingMult, float fStanceMult, float freq) {
        _fFreq     = freq;
        _isComp    = false;
        _iSwingLeg = -1;

        _vecStep     = new Vector(20, 20, 10);
        _stepsPerSec = 1f;
        for (int i = 0; i < _paramLegs.length; i++) {
            _paramLegs[i] = new GaitParam(fSwingMult, fStanceMult);
            //_paramLegs[i].setMult(fSwingMult, fStanceMult);
        }        
    }

    public void    setStep(Vector vecStep)                            { _vecStep = vecStep;         }
    public Vector  getStep()                                          { return _vecStep;            }
    public void    setStepsPerSec(float steps)                        { _stepsPerSec = steps;       }
    public float   getStepsPerSec()                                   { return _stepsPerSec;        }
    
    protected void setSwingOffsets(float[] offsets) {
        for (int i = 0; i < _paramLegs.length; i++) {
            _paramLegs[i].setSwingOffset(offsets[i]);
        }
    }
    
    protected Vector calcMoveRatio(Vector mov) {
        float maxV = max(abs(mov.x), abs(mov.y));
        return new Vector(mov.x / maxV, mov.y / maxV);
    }
    
    public Vector doStep(int leg, float freq, Vector mov, Rotator rot) {
        if (abs(mov.x) == kPRECISION && abs(mov.y) == kPRECISION) {
            _paramLegs[leg].reset();
            return new Vector(0.0f, 0.0f, mov.z);
        }

        _paramLegs[leg].tick(mov, _vecStep, freq, _stepsPerSec);
        Vector r = calcMoveRatio(mov);
        Vector c = new Vector(r.x * _paramLegs[leg].getAmplitude(), r.y * _paramLegs[leg].getAmplitude(), mov.z);
        
        if (_paramLegs[leg].isSwingState()) {
            float w0 = _vecStep.x * Gait.kUNIT_MM / 2 * mov.x;
            float l0 = _vecStep.y * Gait.kUNIT_MM * 4 * mov.y;
            float h0 = _vecStep.z * Gait.kUNIT_MM;
            
            _iSwingLeg       = leg;
            _fSwingAmplitude = sqrt(abs((1 - sq(c.x / w0) - sq(c.y / l0)) * sq(h0)));
            c.z = c.z - _fSwingAmplitude;
        } else {
            if (_isComp && _iSwingLeg >= 0) {
                float pct   = abs(_fSwingAmplitude / _vecStep.z);
                float roll  = (IS_RIGHT_LEG(_iSwingLeg) ? -pct : pct) * 2.0f;
                float pitch = (IS_FRONT_LEG(_iSwingLeg) ? -pct : pct) * 2.0f;
    
                rot.set(rot.yaw, pitch, roll);
            }            
            c.set(-c.x, -c.y, c.z);
        }
        
        print(String.format("tick:%3d, leg:%d, amplitude:%6.1f, swing:%d, (%6.1f, %6.1f)\n", _paramLegs[leg].getTick(), 
            leg, _paramLegs[leg].getAmplitude(), int(_paramLegs[leg].isSwingState()), c.x, c.z));
    
        return new Vector(c.x / Gait.kUNIT_MM, c.y / Gait.kUNIT_MM, c.z / Gait.kUNIT_MM);
    }
    
    abstract public String getName();    
};

/*
***************************************************************************************************
* GaitParam
***************************************************************************************************
*/
public class GaitParam {
    private   float    _fAmplitude;
    private   short    _tick;

    private   boolean  _isSwing;
    private   float    _fCurMult;
    private   float    _fSwingMult;
    private   float    _fStanceMult;
    private   float    _fSwingOffset;
    
    
    public GaitParam(float fSwingMult, float fStanceMult) {
        _tick        = 0;
        _fSwingMult  = fSwingMult;
        _fStanceMult = fStanceMult;
        setSwingState(false);
    }
    
    public void setSwingOffset(float fSwingOffset) {
        _fSwingOffset = fSwingOffset;
        
        if (fSwingOffset == 0) {
            setSwingState(true);
        } else {
            _fCurMult = fSwingOffset;
            _isSwing  = false;
        }
    }
 
    public void setSwingState(boolean en) {
        _isSwing = en;
        if (en) {
            _fCurMult = _fSwingMult;
        } else {
            _fCurMult = _fStanceMult;
        }
    }
    
    protected void tick(Vector mov, Vector step, float freq, float stepsPerSec) {
        float full = round(freq * _fCurMult / stepsPerSec);
        float w0 = step.x * Gait.kUNIT_MM / (2 / max(abs(mov.x), abs(mov.y)));
        float a0 = (w0 * 2) * (float(_tick) / full) - w0;

        _tick++;
        _fAmplitude = a0;
        if (_tick > full) {
            setSwingState(!isSwingState());
            _fAmplitude = -w0;
            _tick = 1;
        }
    }
    
    void reset() {
        _tick = 0;
        setSwingOffset(_fSwingOffset);
    }
    
    void setMult(float fSwingMult, float fStanceMult) {
        _fSwingMult  = fSwingMult;
        _fStanceMult = fStanceMult;
    }
    
    public boolean isSwingState()   { return _isSwing;  }
    public float   getAmplitude()   { return _fAmplitude;  }
    public int     getTick()        { return _tick; }
}

/*
***************************************************************************************************
* GaitTrot
***************************************************************************************************
*/
public class GaitTrot extends Gait {
    public GaitTrot(float freq) {
        super(2.0f, 2.0f, freq);
        setSwingOffsets(new float[] { 0, 2, 0, 2});
    }

    public String getName() { return "Trot"; }
};

/*
***************************************************************************************************
* GaitTripod
***************************************************************************************************
*/
public class GaitLateral extends Gait {
    public GaitLateral(float freq) {
        super(1.0f, 3.0f, freq);
        setSwingOffsets(new float[] { 2.0f, 0.5f, 2.5f, 0.0f });
    }

    public String getName() { return "Lateral"; }
};

/*
***************************************************************************************************
* GaitTripod
***************************************************************************************************
*/
public class GaitDiagonal extends Gait {
    public GaitDiagonal(float freq) {
        super(1.0f, 3.0f, freq);
        setSwingOffsets(new float[] { 1.8f, 3.0f, 1.2f, 0.0f});
    }

    public String getName() { return "Diagonal"; }
};


/*
***************************************************************************************************
* GaitTripod - hexapod
***************************************************************************************************
*/
public class GaitTripod extends Gait {
    public GaitTripod(float freq) {
        super(1.0f, 1.0f, freq); //<>//
        setSwingOffsets(new float[] { 0, 1, 0, 1, 0, 1});
    }

    public String getName() { return "Tripod"; }
};


/*
***************************************************************************************************
* GaitWave - hexapod
***************************************************************************************************
*/
public class GaitWave extends Gait {
    public GaitWave(float freq) {
        super(5.0f, 1.0f, freq);
        setSwingOffsets(new float[] { 2, 1, 0, 3, 4, 5});
    }

    public String getName() { return "Wave"; }

};


/*
***************************************************************************************************
* GaitRipple - hexapod
***************************************************************************************************
*/
public class GaitRipple extends Gait {
    public GaitRipple(float freq) {
        super(2.0f, 1.0f, freq);
        //                            0  1  2  3  4  5
        setSwingOffsets(new float[] { 2, 0, 1, 2, 1, 0});
    }

    public String getName() { return "Wave"; }

};
