# ESP32 QuadRuped
**An open-source 3D-printed quadrupedal robots with ESP32. (SpotMicro and Kangal)**
- **Gait pattern generator using Processing**
- **QuadRuped simulation using Processing**
- **Balancing using IMU**
- **FlyPad, GameSirT1D and Bluetooth LE joystick supported**

## 1.Schematic
<img src="https://github.com/PingguSoft/esp32_quadruped/blob/main/pics/schematic.png?raw=true" width="60%"><br>
- Essential parts
  | part name           | description                          |  unit     |
  |---------------------|--------------------------------------|-----------|
  | ESP32 LOLIN32       | cpu                                  | 1         |
  | PCA9658 module      | servo motor controller               | 1         |
  | SPT5430HV-180W 8.4V | HV servo motor                       | 12        |
  | DC-DC buck converter| 7V-28V to 5V 3A                      | 1         |
  | 1S3P 18650 battery  | lithium ion battery                  | 2         |
  | WS2812 RGB LED      | RGB LED for eyes                     | 2         |
  | Round Rocker Switch | Switch with LED                      | 1         |

- Optional parts
  | part name           | description                          |  unit     |
  |---------------------|--------------------------------------|-----------|
  | HX-2S-JH20          | lithium ion 2S charger               | 1         |
  | PDC004-PD 9V        | usb-c PD decoy module                | 1         |
  | STPS2045C           | power shottky rectifier              | 1         |
  | battery indicator   | Lithium Battery Capacity Indicator   | 1         |
<br><br>

## 2.Hardware Configuration
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
<br><br>

## 3.Simulation
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
<br><br>

## 4.robot control with BLE(bluetooth le) joystick
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

----
# Reference
  - IK model<br>
    [12-DOF Quadrupedal Robot: InverseKinematics by Adham Elarabawy](https://www.adham-e.dev/pdf/IK_Model.pdf)
  - Kangal cad files<br>
    [Diy quadruped robot by Baris ALP](https://grabcad.com/library/diy-quadruped-robot-1)
  - SpotMicro cad files<br>
    [User Fran Ferri models](https://gitlab.com/public-open-source/spotmicroai/3dprinting/-/tree/master/User%20Fran%20Ferri%20models)
