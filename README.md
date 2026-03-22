Botivine — Mobile Manipulator
A 6-DOF robotic arm mounted on a mobile base, built for home gardening tasks like picking, placing pots, and pressing switches.
This was my main undergrad project and honestly the one that gave me the most headaches — and the most satisfaction.

The problem I didn't expect
Everything looked fine on paper. Single ESP32, 6 servos, 2 motors, Wi-Fi control. Should work.
It didn't. The ESP32 kept choking — trying to run a web server, handle motor commands, and drive 6 servos at the same time was too much for a standard loop approach. Commands would lag, servos would jitter, the whole thing felt broken.
After a lot of frustration I found the fix: async WebSockets. Instead of the ESP32 constantly polling for commands in a loop, it just listens and reacts the moment something comes in. The main loop() ended up being literally one line:
cppvoid loop() {
  ws.cleanupClients();
}
Once that clicked, everything worked.

What it does

6-DOF arm — Base, Shoulder, Elbow, Wrist1, Wrist2, Gripper
4-wheel differential drive base
Browser-based control — connect to ESP32's Wi-Fi hotspot, open any browser, control instantly
Independent speed sliders for left/right wheels
Slider control for all 6 joints in real time

No app. No Bluetooth. Just Wi-Fi and a browser.

Hardware
ControllerESP32 (single chip for everything)
Arm servosMG996R — Base, Shoulder, Elbow
Wrist/gripperSG90 — Wrist1, Wrist2, Gripper
DriveL298N + 4× DC motors
PowerExternal 12V for servos (powering from ESP32 causes resets — learned this the hard way)
## Wiring Diagram
<img width="1170" height="572" alt="image" src="https://github.com/user-attachments/assets/bb9ff2cd-44d2-4752-9ce3-59577c19d7ef" />
How to run

Install in Arduino IDE: AsyncTCP, ESP Async WebServer, ESP32Servo
Board: ESP32 Dev Module
Flash idp_final_code.ino
Connect to Wi-Fi: MobileManipulator · password: 12345678
Open browser → 192.168.4.1
