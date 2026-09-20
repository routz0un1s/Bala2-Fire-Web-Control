# Bala2Fire Web Control

A web-based controller for the M5Stack Bala2-Fire (SKU: K014-E) self-balancing robot.

This project adds a browser-based control interface to the Bala2-Fire, allowing it to be controlled from a phone, tablet, or computer. The ESP32 creates its own Wi-Fi access point, so no existing Wi-Fi network or Internet connection is required.

The web interface provides an on-screen joystick as well as dedicated directional buttons for controlling the robot while its balancing and PID control systems remain active.

---

## Features

- Standalone Wi-Fi access point created by the ESP32
- Browser-based control interface
- On-screen joystick
- Forward, backward, left, and right controls
- Stop button
- HTTP-based movement commands
- PID-based balancing and speed control
- Automatic movement command timeout
- Bala2-Fire calibration support
- M5Stack display with balancing-angle waveform

---

## Hardware

This project is designed for the **M5Stack Bala2-Fire (K014-E)**.

The Bala2-Fire consists of the M5Stack Fire and the BALA2 motor base. The system uses an ESP32 as the main application processor, an MPU6886 motion sensor, and a separate STM32F030C8T6 controller in the BALA2 base. The BALA2 uses dual encoder motors and PID control to maintain balance.

For complete hardware information, see the [official M5Stack Bala2-Fire documentation](https://docs.m5stack.com/en/app/bala2fire).

---

## Wi-Fi Setup

When the application starts, the ESP32 creates its own Wi-Fi access point.

| Setting | Value |
|---|---|
| SSID | `Bala2Fire` |
| Password | `bala2fire` |
| Web server port | `80` |

Connect a phone, tablet, or computer to the **Bala2Fire** Wi-Fi network.

The ESP32 uses its SoftAP address for the web server. With the standard ESP32 SoftAP configuration, this is normally:


192.168.4.1

The actual address is also printed to the Serial Monitor when the access point starts.

Using the Web Controller
After connecting to the Bala2Fire Wi-Fi network, open a web browser and navigate to:

http://192.168.4.1

The control page contains an on-screen joystick and directional buttons.

---

  ##Joystick
Drag the joystick in the desired direction to control the Bala2-Fire.

The joystick can provide both forward/backward movement and turning at the same time.

Direction Buttons
The interface provides dedicated buttons for:

-Forward

-Backward

-Left

-Right

-Stop

The buttons send movement commands while they are held.

When the joystick is released, it automatically returns to its center position and sends a stop command.

---

  ##PID Control
The web controller does not replace the Bala2Fire's balancing system.

Movement commands are converted into speed and turning targets. The existing PID control system continues to calculate the motor output required to maintain the robot's balance.

The application uses separate PID control for:

Balance angle

Movement speed

Turning is applied by modifying the left and right motor outputs.

Balance PID
Kp = 24.0
Ki = 0.0
Kd = 90.0

Speed PID
Kp = 15.0
Ki = 0.075
Kd = 0.0

The PID control loop runs approximately every 5 ms (200 Hz).

---

  ##Command Timeout
The controller includes a 300 ms command timeout.

If no movement command is received for 300 ms, the requested speed and turning values are reset to zero.

This means that if communication with the browser is interrupted, an old movement command will not continue indefinitely.

#define CMD_TIMEOUT_MS 300

---

  ##Control Commands
The web interface sends movement commands using HTTP requests.

The command endpoint is:

/cmd?x=<turn>&y=<speed>

Both values range from -100 to 100.

x = turning
y = forward/backward movement

The maximum speed command is:

#define MAX_SPEED_CMD 12.0f

The maximum turning command is:

#define MAX_TURN_CMD 250.0f

For example:

x=0,   y=70   → forward
x=0,   y=-70  → backward
x=70,  y=0    → right
x=-70, y=0    → left
x=0,   y=0    → stop

---

  ##Balancing and Turning
The balance PID and speed PID outputs are combined to produce the motor output.

Turning is then applied differentially:

left motor  = motor output - turn command
right motor = motor output + turn command

The motor PWM output is limited to:

-1023 ... 1023

---

  ##Fall Protection
The controller stops the motors when the robot angle exceeds:

±70 degrees

When this happens:

The motors are stopped.

The encoders are reset.

The speed PID integral is cleared.

---

  ##Calibration
The Bala2-Fire calibration system is initialized during startup using:

#include "calibration.h"

To enter calibration mode, hold Button B while powering on the robot.

The firmware then runs:

calibrationGryo();

The calibrated center angle is used as the balance PID setpoint.

During calibration mode:

Button A increases the balance setpoint.

Button C decreases the balance setpoint.

Button B saves the center angle.

The robot should remain still during the initial IMU calibration.

For the official calibration procedure and further information:

[M5Stack Bala2-Fire Documentation](https://docs.m5stack.com/en/app/bala2fire)

---

  ##Buttons
Button	Function
A	Increase balance setpoint by 0.25
B	Save calibration center angle
C	Decrease balance setpoint by 0.25

Holding Button C during startup enters the charging mode implemented by the firmware.

---


  ##Display
The M5Stack display shows the Bala2-Fire image during startup.

The firmware also displays a live waveform representing the current balancing angle.

The waveform is updated approximately every 10 ms.

---

  ##Serial Monitor
The firmware uses:

115200 baud

During startup, calibration values and the ESP32 reset reason are printed to the Serial Monitor.

Example:

x: ...
y: ...
z: ...
angle: ...
Reset reason: Power-on reset

The reset reason can also help diagnose unexpected resets, including brownout resets.

---

  ##Project Dependencies
The project uses the following libraries and modules:

#define M5STACK_MPU6886

#include <M5Stack.h>
#include <WiFi.h>
#include <WebServer.h>

#include "freertos/FreeRTOS.h"
#include "imu_filter.h"
#include "MadgwickAHRS.h"
#include "bala.h"
#include "pid.h"
#include "calibration.h"
#include "esp_system.h"

Acknowledgements
This project is based on and extends the original M5Stack Bala2Fire Arduino example.

The following components are based on the M5Stack Bala2-Fire example:

PID control implementation

bala.h

pid.h

calibration.h

imu_filter.h

MadgwickAHRS.h

M5Stack.h

The web interface, Wi-Fi access point, joystick, HTTP control commands, command timeout, and related modifications were added by this project.

Original M5Stack source and documentation:

https://github.com/m5stack/M5-ProductExampleCodes/tree/master?tab=readme-ov-file

[M5Stack Bala2-Fire Documentation](https://docs.m5stack.com/en/app/bala2fire)

---


  ##Safety

Before testing:

Keep the robot away from people and obstacles.

Verify the motor directions.

Verify the encoder directions.

Perform calibration before normal operation.

Test at low speed initially.

Keep access to the robot's power switch or battery disconnect.

Do not rely on the 300 ms command timeout as the only emergency stop.

License
MIT License

Copyright (c) 2026 Bala2Fire Web Control contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
