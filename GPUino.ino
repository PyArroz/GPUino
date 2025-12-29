//  ═══════════════════════════════════════════════════════════════
//                     GAMEPORT JOYSTICK READER
//                    CODED BY PYARROZ
//  ═══════════════════════════════════════════════════════════════
//
//                        .   ▄██▄
//                       ▄▀  ██████
//                      ▐▌  ████████▄
//                      ▐██▌▐████████
//                     ▄████▌███████
//                    ███████▐██████▌
//                   ████████▐██████▌
//                  ▐████████▌██████
//                  ▐████████▌█████▌
//                  ▐████████▌█████
//                  ▐████████▌████▌
//                  ▐████████ ████
//                 ▐█████████ ███▌
//                ███████████████▀
//               ████████████████
//              ██████████  ▀███
//            ▄███████████   ▐██▌
//           ▐█████████████▄  ███
//           ████████████████▄▐██▌
//          ██████████████████▌██▌
//         ▐██████████████████▌██
//         ███████████████████▌██
//         ███████████████████ ██
//         ███████████████████ ██
//         ██████████████████  ▀▀
//         ▀████████████████
//
//  Full reading of standard Gameport joystick (4 buttons + 2 joysticks)
//  Arduino Uno + CH340
//  Structured SERIAL output for virtual joystick (vJoy)
//
//  Serial format sent (CSV):
//  axisX,axisY,axis2X,axis2Y,btn1,btn2,btn3,btn4
//  ═══════════════════════════════════════════════════════════════

#include <Arduino.h>

// -------------------- BUTTON PINS --------------------
const int btnPins[4] = {2, 3, 4, 5}; // Buttons 1 to 4
const int ledPin = 13;

// -------------------- AXIS PINS (GAMEPORT - RC) --------------------
const int axisXPin  = A0; // Stick 1 X
const int axisYPin  = A1; // Stick 1 Y
const int axis2XPin = A2; // Stick 2 X
const int axis2YPin = A3; // Stick 2 Y

// -------------------- DEBOUNCE --------------------
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;

bool lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};
bool buttonState[4]     = {HIGH, HIGH, HIGH, HIGH};

// -------------------- AXIS CONFIG --------------------
const unsigned int axisTimeout = 3000; // µs max (adjustable)
const int deadzone = 1; // Deadzone threshold
const int centerOffset = 14; // Center value when stick is at rest

// Smoothing filter
const int numReadings = 5;
float readingsX[numReadings] = {0};
float readingsY[numReadings] = {0};
float readings2X[numReadings] = {0};
float readings2Y[numReadings] = {0};
int readIndex = 0;

// -------------------- GAMEPORT RC READING --------------------
unsigned int readGameportAxis(int pin) {
  // Discharge capacitor
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(5);

  // Measure charge time
  pinMode(pin, INPUT);
  unsigned long start = micros();

  while (digitalRead(pin) == LOW) {
    if (micros() - start > axisTimeout) {
      break; // prevent blocking
    }
  }

  return micros() - start;
}

void setup() {
  for (int i = 0; i < 4; i++) pinMode(btnPins[i], INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  bool anyPressed = false;

  // ---------- BUTTONS WITH DEBOUNCE ----------
  for (int i = 0; i < 4; i++) {
    int reading = digitalRead(btnPins[i]);

    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != buttonState[i]) {
        buttonState[i] = reading;
      }
    }

    lastButtonState[i] = reading;
    if (buttonState[i] == LOW) anyPressed = true;
  }

  // ---------- LED ----------
  digitalWrite(ledPin, anyPressed ? HIGH : LOW);

  // ---------- GAMEPORT AXES WITH SMOOTHING ----------
  // Read raw values
  readingsX[readIndex] = readGameportAxis(axisXPin);
  readingsY[readIndex] = readGameportAxis(axisYPin);
  readings2X[readIndex] = readGameportAxis(axis2XPin);
  readings2Y[readIndex] = readGameportAxis(axis2YPin);
  
  readIndex = (readIndex + 1) % numReadings;
  
  // Calculate averages for smooth values
  float sumX = 0, sumY = 0, sum2X = 0, sum2Y = 0;
  for (int i = 0; i < numReadings; i++) {
    sumX += readingsX[i];
    sumY += readingsY[i];
    sum2X += readings2X[i];
    sum2Y += readings2Y[i];
  }
  
  float axisX = sumX / numReadings;
  float axisY = sumY / numReadings;
  float axis2X = sum2X / numReadings;
  float axis2Y = sum2Y / numReadings;
  
  // Apply center offset and deadzone
  float centeredX = axisX - centerOffset;
  float centeredY = axisY - centerOffset;
  float centered2X = axis2X - centerOffset;
  float centered2Y = axis2Y - centerOffset;
  
  // Apply deadzone
  if (abs(centeredX) < deadzone) centeredX = 0;
  if (abs(centeredY) < deadzone) centeredY = 0;
  if (abs(centered2X) < deadzone) centered2X = 0;
  if (abs(centered2Y) < deadzone) centered2Y = 0;

  // ---------- SERIAL OUTPUT ----------
  Serial.print(centeredX, 2);   Serial.print(",");
  Serial.print(centeredY, 2);   Serial.print(",");
  Serial.print(centered2X, 2);  Serial.print(",");
  Serial.print(centered2Y, 2);  Serial.print(",");
  Serial.print(buttonState[0] == LOW); Serial.print(",");
  Serial.print(buttonState[1] == LOW); Serial.print(",");
  Serial.print(buttonState[2] == LOW); Serial.print(",");
  Serial.println(buttonState[3] == LOW);

  delay(5); // ~200 Hz
}
