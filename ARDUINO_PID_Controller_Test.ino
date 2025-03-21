#include "Arduino_BMI270_BMM150.h"
#include <math.h>
#include <Arduino.h>

// Motor Driver Pins
const int Ain1 = 2;  
const int Ain2 = 3;  
const int Bin2 = 4;  
const int Bin1 = 5;  

// PID Gains (Tune These)
float Kp = 2.0;  
float Ki = 0.0;  
float Kd = 0.0;  

// PID Parameters
float desiredAngle = 0.0;  // Target balance angle
float proportional;
float integral = 0.0;
float derivative;
float currentError;
float prevError = 0.0;
float dt;
unsigned long currentTime;
unsigned long prevTime = 0;
float PIDoutput;
float pwm;
float dutyCycle;

// Complementary Filter Parameters
float Kw = 0.96;  // Complementary filter weight
float filteredAngle;
float accelAngle = 0.0;
float gyroAngle = 0.0;
float gyroChange;
float gyroBias = 0.0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Gyroscope calibration (bias removal)
  float x, y, z;
  int samples = 0;          // Initialize samples counter properly
  while (samples < 1000) {
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(x, y, z);
      gyroBias += x;
      samples++;
    }
    delay(2);
  }
  gyroBias /= samples;

  // Set motor pins as outputs
  pinMode(Ain1, OUTPUT);
  pinMode(Ain2, OUTPUT);
  pinMode(Bin1, OUTPUT);
  pinMode(Bin2, OUTPUT);

  prevTime = micros();
}

void loop() {
  currentTime = micros();
  dt = (currentTime - prevTime) / 1E6;  // Time step in seconds
  prevTime = currentTime;

  // IMU Data
  float ax, ay, az, gx, gy, gz;
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
  } else {
    Serial.println("IMU error");
    delay(100);
    return;
  }

  // Tilt Calculation
  gx -= gyroBias;
  gyroChange = gx * dt;
  gyroAngle += gyroChange;

  accelAngle = -atan2(ay, az) * (180.0 / PI);
  filteredAngle = Kw * (filteredAngle + gyroChange) + (1 - Kw) * accelAngle;

  // PID Control
  currentError = desiredAngle - filteredAngle;

  proportional = Kp * currentError;
  integral += currentError * dt;

  // Anti-windup protection
  integral = constrain(integral, -50.0, 50.0);

  derivative = (currentError - prevError) / dt;
  prevError = currentError;

  PIDoutput = proportional + (Ki * integral) + (Kd * derivative);

  // ✅ Fixed Dead Zone Logic
  const float deadZone = 0.5;  // ±1° dead zone

  if (abs(currentError) < deadZone) {  
    pwm = 0;  // No movement near balance point
    driveMotorsSD(0, 0);

  } else {
    pwm = constrain(abs(PIDoutput), 70, 255);

    // Drive forward/backward based on error direction
    if (PIDoutput > 0) {
      driveMotorsSD(pwm, pwm);   // Forward
    } else {
      driveMotorsSD(-pwm, -pwm);  // Backward
    }
  }

  dutyCycle = (pwm / 255.0) * 100;

  // Debugging
  Serial.print("Angle: "); Serial.print(filteredAngle);
  Serial.print(", PID: "); Serial.print(PIDoutput);
  Serial.print(", PWM: "); Serial.println(pwm);

  delay(5);
}

// ===== FAST DECAY PWM =====
void driveMotorsFD(int speedL, int speedR) {
  // Left Motor
  if (speedL > 0) {   // Forward (Fast Decay)
    analogWrite(Ain1, speedL);  
    analogWrite(Ain2, 0);
  } else {            // Reverse (Fast Decay)
    analogWrite(Ain1, 0);
    analogWrite(Ain2, abs(speedL));
  }
  // Right Motor
  if (speedR > 0) {   // Forward (Fast Decay)
    analogWrite(Bin1, speedR);  
    analogWrite(Bin2, 0);
  } else {            // Reverse (Fast Decay)
    analogWrite(Bin1, 0);
    analogWrite(Bin2, abs(speedR));
  }
}

// ===== SLOW DECAY PWM =====
void driveMotorsSD(int speedL, int speedR) {
  // Left Motor
  if (speedL > 0) {   // Forward (Slow Decay)
    analogWrite(Ain1, 1);  
    analogWrite(Ain2, speedL);
  } else {            // Reverse (Slow Decay)
    analogWrite(Ain1, abs(speedL));
    analogWrite(Ain2, 1);
  }
  // Right Motor
  if (speedR > 0) {   // Forward (Slow Decay)
    analogWrite(Bin1, 1);  
    analogWrite(Bin2, speedR);
  } else {            // Reverse (Slow Decay)
    analogWrite(Bin1, abs(speedR));
    analogWrite(Bin2, 1);
  }
}
