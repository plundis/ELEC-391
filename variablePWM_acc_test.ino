#include <Arduino.h>
#include "Arduino_BMI270_BMM150.h"
#include <math.h>  // Include for atan2()

float x, y, z;
float xangle, yangle, zangle;
float degreesX = 0;
float degreesY = 0;
float theta, theta2;

int variablePWM;

const int Ain1 = 2;  // Ain1 on DRV8833 goes to D2 (pin2 2) on arduino
const int Ain2 = 3;  // Ain2 on DRV8833 goes to D3 (pin2 3) on arduino
const int Bin2 = 4;  // Bin2 on DRV8833 goes to D4 (pin2 4) on arduino
const int Bin1 = 5;  // Bin1 on DRV8833 goes to D5 (pin2 5) on arduino

void setup() {
  pinMode(Ain1, OUTPUT);
  pinMode(Ain2, OUTPUT);
  pinMode(Bin2, OUTPUT);
  pinMode(Bin1, OUTPUT);

  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
}

void loop() {
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    // Calculate tilt in degrees
    // degreesX = atan2(x, sqrt(y * y + z * z)) * 180 / PI;
    // degreesY = atan2(y, sqrt(x * x + z * z)) * 180 / PI;

    // xangle = x * 180 / PI;
    // yangle = y * 180 / PI;
    // zangle = z * 180/ PI;

    theta = -atan2(y,z) * 180 / PI;

    // if (abs(xangle) < 10 && abs(zangle) < 10 && abs(zangle) > 0.01) {
    //     theta2 = xangle / zangle;
    // } else {
    //     theta2 = atan2(xangle, zangle);
    // }

    //variable PWM
    if (theta < 0) {
      variablePWM = 2.8*abs(theta);
      analogWrite(Ain1, 0); // Keep Ain1 low for reverse direction
      analogWrite(Ain2, variablePWM);
      analogWrite(Bin2, variablePWM);
      analogWrite(Bin1, 0); // Keep Bin1 low for reverse direction
    } else {
      variablePWM = 2.8*abs(theta);
      analogWrite(Ain1, variablePWM);
      analogWrite(Ain2, 0); // Keep Ain2 low for forward direction
      analogWrite(Bin2, 0); // Keep Bin2 low for forward direction
      analogWrite(Bin1, variablePWM);
    }


    // Print tilt angles
    // Serial.print("X Tilt: ");
    // Serial.print(x);
    // Serial.print("°, Y Tilt: ");
    // Serial.print(degreesY);
    // Serial.println("°");
   // Serial.print("theta: ");
    Serial.print("Accelerometer angle: ");
  //Serial.print(acc_ang);
    Serial.println(theta);

    //Serial.println("°, theta2: ");
    // Serial.print(theta2);
    // Serial.println("°");
  }

  delay(100);  // Adjust sample rate
}
