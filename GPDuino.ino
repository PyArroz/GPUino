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

// -------------------- GAMEPORT RC READING --------------------
unsigned int readGameportAxis(int pin) {
  // Discharge capacitor
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(10);

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

  // ---------- GAMEPORT AXES ----------
  unsigned int axisX  = readGameportAxis(axisXPin);
  unsigned int axisY  = readGameportAxis(axisYPin);
  unsigned int axis2X = readGameportAxis(axis2XPin);
  unsigned int axis2Y = readGameportAxis(axis2YPin);

  // ---------- SERIAL OUTPUT ----------
  Serial.print(axisX);   Serial.print(",");
  Serial.print(axisY);   Serial.print(",");
  Serial.print(axis2X);  Serial.print(",");
  Serial.print(axis2Y);  Serial.print(",");
  Serial.print(buttonState[0] == LOW); Serial.print(",");
  Serial.print(buttonState[1] == LOW); Serial.print(",");
  Serial.print(buttonState[2] == LOW); Serial.print(",");
  Serial.println(buttonState[3] == LOW);

  delay(10); // ~100 Hz
}
