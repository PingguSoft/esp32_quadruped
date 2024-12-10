boolean   _isCamLog = false;
Vector    _origin;
QuadRuped _quad;

class CamCtrl {
    int        _mode;
    Vector    _eye;
    Vector    _center;
    Vector    _up;
    Vector    _rot;

    Vector  _tblCamFix[][] = {
        { new Vector(   0,   0, 700), new Vector( 0, 0, 0), new Vector(0, 0, 0) },      // up
        { new Vector(-300, 700,   0), new Vector(10, 0, 0), new Vector(-90, 0,  90) },  // right
        { new Vector(-300, 700,   0), new Vector(10, 0, 0), new Vector(-90, 0, -90)  }, // left
        { new Vector(-300, 600,   0), new Vector(10, -600, 0), new Vector(-90, 0, 0) }, // front
    };
    
    CamCtrl() {
        _mode   = 0;
        _eye    = new Vector();
        _center = new Vector(0, 0, 0);
        _up     = new Vector(0, 1, 0);
        _rot    = new Vector(0, 0, 0);
    }
    
    void init() {
        _eye.set(0, 0, (height / 2.0) / tan(radians(30)));
        move(3);
    }
    
    void move(int idx) {
        if (idx < _tblCamFix.length) {
            _eye.set(_tblCamFix[idx][0]);
            _center.set(_tblCamFix[idx][1]);
            _rot.set(_tblCamFix[idx][2]);
        }
    }
    
    void apply() {
        beginCamera();
        camera(_eye.x, _eye.y, _eye.z, _center.x, _center.y, _center.z, _up.x, _up.y, _up.z);
        endCamera();
        
        rotateX(radians(_rot.y));
        rotateY(radians(_rot.x));
        rotateZ(radians(_rot.z));
    }
    
    boolean process(int key) {
        boolean ret = true;
        Vector   v = _eye;
        
        //print(String.format("keyCode:%d, key:%d\n", keyCode, key));
        
        if (_mode == 1)
            v = _center;
        else if (_mode == 2)
            v = _rot;
        
        switch (keyCode) {
        case UP:
            if (_mode == 2)
                v.y = (v.y > -360) ? (v.y - 10) : -360;
            else
                v.y -= 10;
            break;
            
        case DOWN:
            if (_mode == 2)
                v.y = (v.y < 360) ? (v.y + 10) : 360;
            else
                v.y += 10;
            break;
            
        case LEFT:
            if (_mode == 2)
                v.x = (v.x > -360) ? (v.x - 10) : -360;
            else
                v.x -= 10;
            break;
            
        case RIGHT:
            if (_mode == 2)
                v.x = (v.x < 360) ? (v.x + 10) : 360;
            else
                v.x += 10;
            break;
            
        case 2:        // home
            if (_mode == 2)
                v.z = (v.z > -360) ? (v.z - 10) : -360;
            else
                v.z = (v.z > 10) ? (v.z - 10) : 0;
            break;
            
        case 3:        // end
            if (_mode == 2)
                v.z = (v.z < 360) ? (v.z + 10) : 360;
            else
                v.z += 10;        
            break;
            
        case 147:      // delete
            _mode = (_mode + 1) % 3;
            print(String.format("MODE:%d\n", _mode));
            break;
            
        case 11:        // pgdn
            break;
            
        case 26:        // insert
            break;
            
        case 16:        // pgup
            break;
            
        default:
            switch (key) {
            case '1':
            case '2':
            case '3':
            case '4':
                move(key - '1');
                break;            
                
            default:
                ret = false;
                break;
            };
            break;
        }
        
        if (ret) {
            print(String.format("CAM : eye(%5.1f %5.1f %5.1f), center(%5.1f %5.1f %5.1f), rot(%5.1f %5.1f %5.1f)\n", 
                _eye.x, _eye.y, _eye.z, _center.x, _center.y, _center.z, _rot.x, _rot.y, _rot.z));
        }
        
        return ret;
    }    
}

CamCtrl _cam    = new CamCtrl();
boolean _isWalk = false;
Object  _mutex  = new Object();

//static final float kBodyWidth       = 110;
//static final float kBodyHeight      = 300;
//static final float kCoxaLength      = 60;
//static final float kCoxaOffsetZ     = 1;
//static final float kFemurLength     = 115;
//static final float kTibiaLength     = 135;

static final float kBodyWidth       = 95;
static final float kBodyHeight      = 200;
static final float kCoxaLength      = 40;
static final float kCoxaOffsetZ     = 0;
static final float kFemurLength     = 100;
static final float kTibiaLength     = 115;

void setup() {
    hint(ENABLE_KEY_REPEAT);
    size(1280, 720, P3D);
    lights();
    //noCursor();
    frameRate(60);
    //fullScreen(P3D);
    com.jogamp.newt.opengl.GLWindow window = (com.jogamp.newt.opengl.GLWindow)(surface.getNative());
    window.setResizable(true);
    window.setMaximized(true, true);    
    
    _cam.init();
    _origin  = new Vector(width / 2, height / 2, 0);
    _quad = new QuadRuped(kBodyWidth, kBodyHeight, kCoxaLength, kCoxaOffsetZ, kFemurLength, kTibiaLength);
}

private final color kCOLOR_PLANE = color(100, 100, 100);

void drawPlane(float x, float y, float z, float w, float l) {
    pushMatrix();
    translate(x, y, z);
    fill(kCOLOR_PLANE);
    box(w, l, 1);
    popMatrix();
}

void draw() {
    _cam.apply();
    background(0);
    drawPlane(_origin.x, _origin.y, _origin.z, 5000, 5000);
    _quad.update(_isWalk);
}

void keyPressed() {
   
    if (_cam.process(key))
        return;
    
    switch (key) {
    case 'j':
        _quad.getRot().roll--;
        break;

    case 'l':
        _quad.getRot().roll++;
        break;
        
    case 'i':
        _quad.getRot().pitch++;
        break;

    case 'k':
        _quad.getRot().pitch--;
        break;        

    case 'u':
        _quad.getRot().yaw--;
        break;

    case 'o':
        _quad.getRot().yaw++;
        break;

    case 'p':
        _quad.getRot().zero();
        _quad.getPos().zero();
        break;

    case '0':
        _isCamLog = !_isCamLog;
        break;

    case 'q':
        _quad.getPos().z--;
        break;

    case 'e':
        _quad.getPos().z++;
        break;
        
    case 'w':
        _quad.getPos().y++;
        break;
        
    case 's':
        _quad.getPos().y--;
        break;
        
    case 'a':
        _quad.getPos().x--;
        break;
        
    case 'd':
        _quad.getPos().x++;
        break;
        
    case ' ':
        _isWalk = !_isWalk;
        print(String.format("Waling mode:%d\n", int(_isWalk)));
        break;
        
    case '=':
        _quad._gait.setStepsPerSec(_quad._gait.getStepsPerSec() + 1);
        print(String.format("Steps Per Sec:%f\n", _quad._gait.getStepsPerSec()));
        break;
        
    case '-':
        if (_quad._gait.getStepsPerSec() > 1) {
            _quad._gait.setStepsPerSec(_quad._gait.getStepsPerSec() - 1);
            print(String.format("Steps Per Sec:%f\n", _quad._gait.getStepsPerSec()));
        }
        
        break;
    }
    //_quad.update(_isWalk);
    //print(String.format("P%s R%s\n", _quad.getPos().toString(), _quad.getRot().toString()));
}
