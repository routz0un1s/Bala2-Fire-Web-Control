  # Bala2Fire Web Control

 A web-based controller for the M5Stack Bala2-Fire (SKU: K014-E) self-balancing robot.

 This project adds a browser-based control interface to the Bala2-Fire, allowing it to be controlled from a phone, tablet, or computer. The ESP32 creates its own Wi-Fi access point, so no existing Wi-Fi network or Internet connection is required.

 The web interface provides an on-screen joystick as well as dedicated directional buttons for controlling the robot while its balancing and PID control systems remain active.

---

 # Features

 - Standalone Wi-Fi access point created by the ESP32
- Browser-based control interface
- On-screen joystick
- Forward, backward, left, and right controls, as well as a stop button
- HTTP-based movement commands
- PID-based balancing and speed control
- Automatic movement command timeout
- Bala2-Fire calibration support
- M5Stack display with balancing-angle waveform

---

 # Hardware

 This project is designed for the **M5Stack Bala2-Fire (K014-E)**.

 The Bala2-Fire consists of the M5Stack Fire and the BALA2 motor base. The system uses an ESP32 as the main application processor, an MPU6886 motion sensor, and a separate STM32F030C8T6 controller in the BALA2 base. The BALA2 uses dual encoder motors and PID control to maintain balance.

 For complete hardware specifications and documentation, see the official M5Stack documentation:

 - M5Stack Bala2-Fire documentation
- M5Stack Bala2-Fire GitHub repository

 The official M5Stack documentation identifies the Bala2-Fire as SKU **K014-E**, with an ESP32, MPU6886, STM32F030C8T6 base controller, dual encoder motors, and PID-based balancing.  docs.m5stack.com+1

---

 # Wi-Fi Setup

 When the application starts, the ESP32 creates its own Wi-Fi access point.

 ### Default Network

 | Setting | Value |
| --- | --- |
| SSID | `Bala2Fire` |
| Password | `bala2fire` |
| Web server port | `80` |

Connect a phone, tablet, or computer to the **Bala2Fire** Wi-Fi network.

 The ESP32 uses its SoftAP address for the web server. With the standard ESP32 SoftAP configuration, this is normally:

```
192.168.4.1
```

 The actual address is also printed to the Serial Monitor when the access point starts.

---

 # Using the Web Controller

 After connecting to the Bala2Fire Wi-Fi network, open a web browser and navigate to:

```
http://192.168.4.1
```

 The control page contains an on-screen joystick and directional buttons.

 ## Joystick

 Drag the joystick in the desired direction to control the Bala2-Fire.

 The joystick can provide both forward/backward movement and turning at the same time.

 ## Direction Buttons

 The interface also provides dedicated buttons for:

 - Forward
- Backward
- Left
- Right

 The buttons send movement commands while they are held.

 The center button is the **Stop** button.

 When the joystick is released, it automatically returns to its center position and sends a stop command.

---

 # PID Control

 The web controller does not replace the Bala2-Fire's balancing system.

 Movement commands are converted into speed and turning targets. The existing PID control system continues to calculate the motor output required to maintain the robot's balance.

 The application uses separate PID control for:

 - Balance angle
- Movement speed

 Turning is applied by modifying the left and right motor outputs.

 The default balance PID parameters in this project are:

```
Kp = 24.0
Ki = 0.0
Kd = 90.0
```

 The speed PID parameters are:

```
Kp = 15.0
Ki = 0.075
Kd = 0.0
```

---

 # Command Timeout

 The controller includes a **300 ms command timeout**.

 If no movement command is received for 300 ms, the requested speed and turning values are reset to zero.

 This means that if communication with the browser is interrupted, an old movement command will not continue indefinitely.

 The timeout is defined in the firmware as:

```
#define CMD_TIMEOUT_MS 300
```

---

 # Control Commands

 The web interface sends movement commands using HTTP requests.

 The command endpoint is:

```
/cmd?x=<turn>&y=<speed>
```

 Both values are limited to:

```
-100 ... 100
```

 where:

```
x = turning
y = forward/backward movement
```

 The firmware converts these values into internal speed and turning targets.

 The maximum speed command is:

```
#define MAX_SPEED_CMD 12.0f
```

 The maximum turning command is:

```
#define MAX_TURN_CMD 250.0f
```

 The conversion is:

```
speed target = y / 100 × 12.0

turn target = x / 100 × 250.0
```

 For example:

```
x=0,   y=70   → forward movement
x=0,   y=-70  → backward movement
x=70,  y=0    → right turn
x=-70, y=0    → left turn
x=0,   y=0    → stop
```

 The firmware constrains incoming values before applying them to the control system.

---

 # Balancing System

 The balancing system runs independently from the web interface.

 The robot continuously reads its angle from the MPU6886 IMU and uses the balance PID controller to keep the robot upright.

 The initial balance setpoint in the firmware is:

```
static float angle_point = -1.5;
```

 During startup, the calibrated center angle replaces this initial value.

 The balance PID output is limited to:

```
-1023 ... 1023
```

 The control loop runs at approximately 5 ms per iteration, corresponding to approximately 200 Hz.

---

 # Speed Control

 A second PID controller is used to control the robot's movement speed.

 The speed controller uses the wheel encoder feedback from the Bala2-Fire.

 The encoder values from the left and right wheels are combined to calculate the movement speed.

 A simple filtering stage is applied to the encoder measurement before it is used by the speed PID.

 The speed controller then combines its output with the balance controller output.

```
motor output = balance output + speed output
```

---

 # Turning

 Turning is implemented using differential motor control.

 The requested turning value is added to one motor and subtracted from the other.

```
left motor  = motor output - turn command
right motor = motor output + turn command
```

 Both motor outputs are constrained to:

```
-1023 ... 1023
```

 This allows the robot to maintain balance while turning.

---

 # Fall Protection

 The controller stops the motors when the robot angle exceeds the configured operating range.

 The current limit is:

```
±70 degrees
```

 When the measured angle exceeds this range:

 - The motors are stopped.
- The wheel encoders are reset.
- The speed PID integral is cleared.

 This prevents the controller from continuing normal motor control when the robot has fallen outside its operating angle.

---

 # Calibration

 The Bala2-Fire calibration system is initialized during startup.

 The firmware retrieves calibration information including:

 - X-axis offset
- Y-axis offset
- Z-axis offset
- Center balance angle

 The calibrated center angle is then used as the balance PID setpoint.

 To enter calibration mode with this project, hold **Button B** while powering on the M5Stack.

 The firmware detects Button B during startup and runs the gyro calibration routine.

 When calibration mode is active, the display shows:

```
calibration mode
```

 The balance center can then be adjusted using Buttons A and C, and Button B saves the center angle.

 ### Official M5Stack Calibration Instructions

 The calibration procedure follows the Bala2-Fire calibration workflow documented by M5Stack.

 The official instructions describe:

 1. Place the BALA2 on a horizontal surface.
2. Keep Button B pressed while powering on.
3. Release the button when the screen lights up.
4. Do not move the robot while the IMU data is acquired.
5. Enter calibration mode.
6. Use Buttons A and C to adjust the correction value.
7. Press Button B to save the parameter.

 See the official M5Stack calibration guide:

 - BALA2 Calibration — M5Stack Quick Start

 The official Bala2-Fire product documentation also provides the calibration procedure and demonstration:

 - M5Stack Bala2-Fire — Usage and Sensor Calibration

 M5Stack states that the Bala2-Fire is factory-calibrated, but provides this procedure for manual recalibration when required.  docs.m5stack.com+1

---

 # M5Stack Buttons

 The three M5Stack buttons are used for calibration and balance-setpoint adjustment.

 ### Button A

 Increases the balance setpoint by:

```
+0.25 degrees
```

 ### Button B

 During calibration mode, saves the current center angle.

 ### Button C

 Decreases the balance setpoint by:

```
-0.25 degrees
```

 Button C also has a special startup function for charging mode.

---

 # Charge Mode

 If Button C is already pressed during startup, the firmware enters charge mode.

 The display shows:

```
Charge mode
```

 The firmware waits for the M5Stack to report that charging has started.

 The display then shows:

```
Start charging...
```

 When the battery reports that charging is complete:

```
Charge completed!
```

 The firmware remains in charge mode until the device is restarted.

---

 # Display

 The M5Stack display shows the Bala2-Fire graphic during normal operation.

 The firmware also displays a real-time waveform representing the measured balancing angle.

 The waveform is updated approximately every 10 ms.

 The angle is obtained using:

```
getAngle()
```

 The waveform provides a simple visual indication of the robot's current balance behavior.

---

 # Control Loop

 The main PID control task runs on a dedicated FreeRTOS task.

 The PID loop runs with a target period of approximately:

```
5 ms
```

 or approximately:

```
200 Hz
```

 The PID task is responsible for:

 - Reading the current balance angle
- Updating wheel encoder values
- Calculating movement speed
- Running the balance PID
- Running the speed PID
- Applying the turning command
- Sending the final PWM values to the motors

 The main Arduino loop is responsible for:

 - Processing web server requests
- Updating the display
- Reading M5Stack button presses
- Adjusting the balance setpoint

---

 # Project Dependencies and M5Stack References

 This project uses functionality from the M5Stack Arduino environment.

 The main external libraries used by the supplied firmware are:

```
#include <M5Stack.h>
#include <WiFi.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
```

 The project also includes its own local modules:

```
imu_filter.h
MadgwickAHRS.h
bala.h
pid.h
calibration.h
```

 The `M5Stack.h` functionality, including M5Stack initialization, buttons, display, power management, and IMU access, is provided by the M5Stack Arduino library.

 M5Stack's official repository documents the `M5Stack` library and its included examples:

 - M5Stack Arduino Library — GitHub

 The M5Stack library documentation specifically describes the use of `M5Stack.h`, `M5.begin()`, the hardware buttons, power functions, and IMU support.  GitHub+1

 The MPU6886 implementation used by the M5Stack library is also maintained by M5Stack:

 - M5Stack MPU6886 implementation

 The official M5Stack Bala2/Bala2-Fire repository provides the original Bala2/Bala2-Fire firmware and example project:

 - M5Stack M5Bala2 repository

 This project builds upon the M5Stack/Bala2-Fire software and hardware ecosystem while adding the browser-based Wi-Fi control interface and associated control functionality.

---

 # Acknowledgements

 This project is based on and extends functionality provided by the M5Stack Bala2-Fire platform and M5Stack's example/software ecosystem.

 In particular, references are made to:

 - **M5Stack Arduino Library** — used for the M5Stack hardware interface and functionality.
- **M5Stack Bala2/Bala2-Fire firmware and examples** — used as the basis for Bala2-Fire functionality.
- **M5Stack MPU6886 support** — used for IMU access.
- **M5Stack Bala2-Fire calibration documentation** — used as the reference for the calibration procedure.

 Official resources:

 - M5Stack Arduino Library
- M5Stack Bala2/Bala2-Fire repository
- M5Stack Bala2-Fire documentation
- M5Stack BALA2 calibration guide
- M5Stack MPU6886 source

 The M5Stack `M5Bala2` repository is itself released under the MIT license.  GitHub

---

 # Serial Monitor

 The firmware uses the ESP32 Serial interface at:

```
115200 baud
```

 During startup it prints the calibration values:

```
x: ...
y: ...
z: ...
angle: ...
```

 It also reports the ESP32 reset reason.

 Possible reset reasons include:

```
Power-on reset
BROWNOUT RESET (power sag!)
Software panic / crash
Interrupt watchdog timeout
Task watchdog timeout
Other watchdog timeout
Software reset
Woke from deep sleep
External pin reset
Unknown
```

 The reset information can be useful when diagnosing unexpected restarts, particularly brownout resets.

---

 # Web Server

 The ESP32 runs a web server on port `80`.

 The available endpoints are:

 ### `/`

 Returns the control interface.

```
GET /
```

 ### `/cmd`

 Updates the movement and turning commands.

```
GET /cmd?x=<x>&y=<y>
```

 For example:

```
/cmd?x=0&y=70
```

 requests forward movement.

```
/cmd?x=-70&y=0
```

 requests a left turn.

```
/cmd?x=0&y=0
```

 requests zero movement and zero turning.

---

 # Browser Compatibility

 The control interface is designed for desktop and mobile browsers.

 It uses:

 - HTML
- CSS
- JavaScript
- Mouse events
- Touch events
- HTTP requests

 No external JavaScript libraries or Internet connection are required.

 The complete control page is embedded directly into the ESP32 firmware, so the robot can be controlled locally without Internet access.

---

 # Safety

 This project controls a physical self-balancing robot.

 Before testing the robot:

 - Keep the robot away from people and obstacles.
- Verify the motor directions.
- Verify the encoder directions.
- Verify that the IMU is working correctly.
- Perform calibration before normal operation.
- Test the robot at low speed initially.
- Keep access to the robot's power switch or battery disconnect.
- Do not rely on the web command timeout as the only emergency stop.

 The web controller provides movement commands; the balancing and motor control continue to run locally on the robot.

---

 # Project Structure

 A typical project structure is:

```
Bala2Fire-Web-Control/
│
├── Bala2Fire-Web-Control.ino
├── bala.h
├── pid.h
├── calibration.h
├── imu_filter.h
├── MadgwickAHRS.h
│
└── README.md
```

 The exact source-file structure may vary depending on how the Bala2-Fire firmware modules are organized.

---

 # How It Works

 The overall control flow is:

```
Phone / Tablet / Computer
          |
          | Wi-Fi
          v
     ESP32 Web Server
          |
          | x / y commands
          v
   Movement + Turn Targets
          |
          +----------------+
          |                |
          v                v
      Speed PID       Balance PID
          |                |
          +-------+--------+
                  |
                  v
          Motor PWM Output
                  |
          v                v
       Left Motor      Right Motor
                  |
                  v
              Encoders
                  |
                  +-------> Speed Feedback

MPU6886
   |
   v
Balance Angle
   |
   +---------------------> Balance PID
```

 The browser therefore controls the robot's desired movement, while the ESP32 continues to perform the high-frequency balancing and motor control locally.

---

 # License

 MIT License

 Copyright (c) 2026 Bala2Fire Web Control contributors

 Permission is hereby granted, free of charge, to any person obtaining a copy\
 of this software and associated documentation files (the "Software"), to deal\
 in the Software without restriction, including without limitation the rights\
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\
 copies of the Software, and to permit persons to whom the Software is\
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all\
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\
 SOFTWARE.

---

 # Third-Party Software

 This project uses or builds upon software and documentation from M5Stack.

 M5Stack's Bala2/Bala2-Fire repository is MIT licensed. The M5Stack Arduino library and other referenced M5Stack components are distributed under their respective licenses. See the original repositories for their complete license and copyright notices.

 - M5Stack Arduino Library — License and source
- M5Stack M5Bala2 — License and source

 This project is an independent web-control extension for the M5Stack Bala2-Fire platform and is not an official M5Stack product.
