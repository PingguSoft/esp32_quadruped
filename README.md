# ESP32 QuadRuped
**An open-source 3D-printed quadrupedal robots with ESP32. (SpotMicro and Kangal)**
- **Gait pattern generator using Processing**
- **QuadRuped simulation using Processing**
- **Balancing using IMU**
- **FlyPad, GameSirT1D and Bluetooth LE joystick supported**

## Schematic
<img src="https://github.com/PingguSoft/esp32_quadruped/blob/main/pics/schematic.png?raw=true" width="60%">

## Hardware Configuration
<img src="https://github.com/PingguSoft/esp32_quadruped/blob/main/pics/quadruped_top.png?raw=true" width="60%">

- ### Kangal
  <div>
    <a href="https://www.youtube.com/watch?v=2IXcY3YnklY"><img src="https://img.youtube.com/vi/2IXcY3YnklY/0.jpg"/></a>
  </div>
  
  *click to youtube video*

- ### SpotMicro
  <div>
    <a href="https://www.youtube.com/watch?v=9ieQvUIGtjo"><img src="https://img.youtube.com/vi/9ieQvUIGtjo/0.jpg"/></a>
  </div>
  
  *click to youtube video*

## Simulation
 - pose and walking simulation with [processing](https://processing.org/).
   
   <img src="https://github.com/PingguSoft/esp32_quadruped/blob/main/pics/simulation.gif?raw=true" width="60%">
    
    **moving keys**
    | key       | camera        |  key      | quadruped control |  
    |-----------|---------------|-----------|-------------------|
    | **1**     | Top View      | **j / l** | roll- / roll+     |
    | **2**     | Right View    | **k / i** | pitch- / roll+    |
    | **3**     | Left View     | **u / o** | yaw- / yaw+       |
    | **4**     | Front View    | **p**     | reset             |
    | **up**    | camera up     | **q / e** | z- / z+           |
    | **down**  | camera down   | **w / s** | y- / y+           |
    | **left**  | camera left   | **a / d** | x- / x+           |
    | **right** | camera right  | **w / s** | y- / y+           |
    | **home**  | camera z up   | **- / =** | step- / step+     |
    | **down**  | camera z down | **space** | walk / stop       |

 - gait pattern generation with [processing](https://processing.org/)
 
   <img src="https://github.com/PingguSoft/esp32_quadruped/blob/main/pics/gait_pattern.gif?raw=true" width="60%">

## robot control with BLE(bluetooth le) joystick
  | key        |    control    |  key      |  control          |
  |----------- |---------------|-----------|-------------------|
  | **A**      | walk / stop   | **L2+L1** | step interval-    |
  | **B**      | gait change   | **L2+R1** | step interval+    |
  | **X**      | balance mode  | **L2+X**  | step height-      |
  | **Y**      | flash ledstrip| **L2+Y**  | step height+      |
  | **THUMB-L**| save settings | **L2+A**  | step dist-        |
  | **THUMB-R**| load settings | **L2+B**  | step dist+        |
  | **DPAD-L** | toe offset-   | **R2+L1** | toe offset-       |
  | **DPAD-R** | toe offset+   | **R2+R1** | toe offset+       |
