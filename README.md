# GPUino
Enables a classic gameport controller to work on Windows 11 using an Arduino UNO and vJoy by converting gameport inputs to a USB virtual joystick.
macOS and Linux versions are planned.

This program allows the use of a classic Gameport controller on Windows 11 by converting its signals to USB using an Arduino UNO (CH340).
The Arduino reads the analog and digital inputs from the gameport controller and sends the data via serial communication to the PC.

A Python application receives this serial data, processes the joystick axes and buttons, and maps them to a virtual joystick using vJoy.
This makes the controller appear to Windows as a standard USB game controller, fully compatible with modern games and applications.

The program includes a simple graphical interface to:

Select and connect to the Arduino COM port

Read and display raw input data

Map analog axes and buttons to vJoy

Optionally swap joystick axes in real time

This solution enables legacy gameport hardware to be used seamlessly on modern Windows systems without native gameport support.
