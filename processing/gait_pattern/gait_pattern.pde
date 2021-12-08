
final color kCOLOR_1  = color(50, 150, 220);
final color kCOLOR_2  = color(255,  0,   0);
final boolean kIS_ANIMATE_KEY = false;
final int   kFREQ     = 60;

Vector  _mov   = new Vector(Gait.kPRECISION + 1.5f, Gait.kPRECISION, 50);
Rotator _rot   = new Rotator(0.0f, 0, 0);
boolean  _done = false;
int      _ctr  = 0;
LegGraph _graph[] = new LegGraph[4];
boolean  _start   = true;
Gait     _gait    = new GaitTrot(kFREQ);

void setup() {
    hint(ENABLE_KEY_REPEAT);
    size(1280, 720);
    frameRate(30);

    _gait.setStepsPerSec(2);
    _graph[0] = new LegGraph(0, width / 2 +  50,  400); //<>//
    _graph[1] = new LegGraph(1, width / 2 - 300,  400);
    _graph[2] = new LegGraph(2, width / 2 - 300,  200);
    _graph[3] = new LegGraph(3, width / 2 +  50,  200);
    
    background(0);
}

void draw() {
    if (_start) {
        background(0);
        _start = false;
    } else {
        if (!kIS_ANIMATE_KEY) {
            for (int i = 0; i < _graph.length; i++) {
                _graph[i].draw();
            }
        }
    }
}

class LegGraph {
    int _pos;
    int _x;
    int _y;
    boolean _cidx;
    short _last;
    int _oldPx;
    int _oldPy;
    
    LegGraph(int leg, int x, int y) {
        _pos = leg;
        _x   = x;
        _y   = y;
        _last = -1;
        _oldPx = Integer.MAX_VALUE;
        _oldPy = Integer.MAX_VALUE;
    }
    
    void draw() {
        strokeWeight(10);
        stroke(kCOLOR_1);
        noFill();
        rect(_x - 100, _y, 300, 200);
        
        Vector vec;
        vec = _gait.doStep(_pos, kFREQ, _mov, _rot);

        if (_oldPx != Integer.MAX_VALUE && _oldPx != Integer.MAX_VALUE) {
            stroke(kCOLOR_1);
            point(_x + 50 + _oldPx, _y - 70 + _oldPy);            
        }
        
        stroke(kCOLOR_2);
        point(_x + 50 + int(vec.x * 5), _y - 70 + int(vec.z * 5));
        
        _oldPx = int(vec.x * 5);
        _oldPy = int(vec.z * 5);
    }
}



void keyPressed() {
    if (kIS_ANIMATE_KEY) {    
        if (key == ' ') {
            for (int i = 0; i < _graph.length; i++) {
                _graph[i].draw();
            }
        }
    }
}
