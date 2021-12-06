public abstract class Gait {
    public final static float kUNIT_MM   = 1f;
    public final static float kPRECISION = 0.1f;

    protected Vector    _vecStep;
    protected float     _stepsPerSec;
    protected GaitParam _paramLegs[] = new GaitParam[4];
    protected float     _fFreq;
    protected float     _fSwingAmplitude;
    protected int       _iSwingLeg;
    protected boolean   _isComp;
    
    public Gait(float fSwingMult, float fStanceMult, float freq) {
        _vecStep     = new Vector(20, 20, 40);
        _stepsPerSec = 1f;
        _fFreq       = freq;
        _isComp      = false;
        _iSwingLeg   = -1;
        
        for (int i = 0; i < _paramLegs.length; i++) {
            _paramLegs[i] = new GaitParam(fSwingMult, fStanceMult);
        }
    }

    public void    setComp(boolean en)        { _isComp  = en;              }
    public void    setStep(Vector vecStep)    { _vecStep = vecStep;         }
    public Vector  getStep()                  { return _vecStep;            }
    public float   getStepsPerSec()           { return _stepsPerSec;        }
    
    public void setStepsPerSec(float steps) {
        for (int i = 0; i < _paramLegs.length; i++) {
            _paramLegs[i].reset();
        }
        _stepsPerSec = steps;
    }
    
    protected void setSwingOffsets(float[] offsets) {
        for (int i = 0; i < _paramLegs.length; i++) {
            _paramLegs[i].setSwingOffset(offsets[i]);
        }
    }
    
    protected Vector calcDirRatio(Vector dir) {
        float maxV = max(abs(dir.x), abs(dir.y));
        return new Vector(dir.x / maxV, dir.y / maxV, dir.z);
    }
    
    public void doStep(int leg, Vector dir, Rotator rot) {
        
        if (abs(dir.x) == kPRECISION && abs(dir.y) == kPRECISION) {
            dir.set(0.0f, 0.0f, dir.z);
            return;
        }

        _paramLegs[leg].tick(dir, _vecStep, _fFreq, _stepsPerSec);
        int    s = 1;// (leg < 2) ? 1 : -1;
        Vector r = calcDirRatio(dir);
        Vector c = new Vector(r.x * _paramLegs[leg].getAmplitude(), s * r.y * _paramLegs[leg].getAmplitude(), dir.z);
        
        if (_paramLegs[leg].isSwingState()) {
            float w0 = _vecStep.x * Gait.kUNIT_MM * 4 * dir.x;
            float l0 = _vecStep.y * Gait.kUNIT_MM / 2 * dir.y;
            float h0 = _vecStep.z * Gait.kUNIT_MM;
            _iSwingLeg = leg;
            _fSwingAmplitude = sqrt(abs((1 - sq(c.x / w0) - sq(c.y / l0)) * sq(h0)));
            c.z = c.z - _fSwingAmplitude;
        } else {
            if (_isComp && _iSwingLeg >= 0) {
                float pct   = abs(_fSwingAmplitude / _vecStep.z);
                float roll  = ((_iSwingLeg >= 2) ? pct : -pct) * 2.0f;
                float pitch = ((_iSwingLeg == 1 || _iSwingLeg == 2) ? pct : -pct) * 2.0f;
                
                rot.set(rot.yaw, pitch, roll);
            }
            c.set(-c.x, -c.y, c.z);
        }
        
        //if (leg == 0)
        //print(String.format("tick:%3d, amplitude:%6.1f, swing:%d, (%6.1f, %6.1f)\n", _paramLegs[leg].getTick(), 
        //    _paramLegs[leg].getAmplitude(), int(_paramLegs[leg].isSwingState()), c.x, c.z));
    
        dir.set(c.x / Gait.kUNIT_MM, c.y / Gait.kUNIT_MM, c.z / Gait.kUNIT_MM);
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
    private   int      _tick;

    private   boolean  _isSwing;
    private   float    _fCurMult;
    private   float    _fSwingMult;
    private   float    _fStanceMult;
    private   float    _fSwingOffset;
    
    public GaitParam(float fSwingMult, float fStanceMult) {
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
    
    protected void tick(Vector dir, Vector step, float freq, float stepsPerSec) {
        float full = round(freq * _fCurMult / stepsPerSec);
        float w0   = step.x * Gait.kUNIT_MM / (2 / max(abs(dir.x), abs(dir.y)));
        float a0   = (w0 * 2) * (float(_tick) / full) - w0;

        _tick++;
        _fAmplitude = a0;
        if (_tick > full) {
            setSwingState(!isSwingState());
            _fAmplitude = -w0;
            _tick = 1;
        }
    }
    
    public void reset() { 
        _tick = 0;
        setSwingOffset(_fSwingOffset);
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
        super(1.0f, 1.0f, freq);
        setSwingOffsets(new float[] { 0, 1, 0, 1 });
    }

    public String getName() { return "Trot"; }
};


/*
***************************************************************************************************
* GaitCrawl
***************************************************************************************************
*/
public class GaitCrawl extends Gait {
    public GaitCrawl(float freq) {
        super(1.0f, 3.0f, freq);
        setSwingOffsets(new float[] { 0, 3, 1, 2 });
        setComp(true);
    }

    public String getName() { return "Crawl"; }

};


/*
***************************************************************************************************
* GaitRipple
***************************************************************************************************
*/
