#include "Arduino_BMI270_BMM150.h"
#include <math.h>  // Include for atan2()
#include <Arduino.h>

// Motor Driver Pins
// LEFT MOTOR
const int Ain1 = 2;  // Ain1 on DRV8833 goes to D2 on Arduino
const int Ain2 = 3;  // Ain2 on DRV8833 goes to D3 on Arduino
// RIGHT MOTOR
const int Bin2 = 4;  // Bin2 on DRV8833 goes to D4 on arduino
const int Bin1 = 5;  // Bin1 on DRV8833 goes to D5 on arduino

// PID Gains (Tune These)
float Kp = 15.0;  
float Ki = 0.5;
float Kd = 2.0;

float desiredAngle = 0.0;  // Desired tilt angle (balance point)
float proportional;
float integral = 0.0;
float derivative;
float error;
float prevError = 0.0;
float currentTime;
float prevTime = 0.0;
float outputPID;

// Complementary Filter Parameters
float filteredAngle = 0.0;
float Kw = 0.95;  // Complementary filter weight

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");
  
  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  
  // Set motor pins as outputs
  pinMode(Ain1, OUTPUT);
  pinMode(Ain2, OUTPUT);
  pinMode(Bin2, OUTPUT);
  pinMode(Bin1, OUTPUT);

  // Optional: Calculate gyroscope bias over time to subtract it from the readings
  for (int i = 0; i < 1000; i++) {
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(x, y, z);
      gyro_bias += x;   // Accumulate the x-axis gyroscope values
      delay(10);        // Wait to gather enough samples
    }
  }
  gyro_bias /= 1000.0;  // Average the bias value
}

void loop() {
  static float prevTime = millis() / 1000.0;  // Store time in seconds
  
  // ===== Read IMU Data =====
  float ax, ay, az, gx, gy, gz;
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
  } else {
    return;  // IMU data is not available
  }

  // ===== Time Calculations =====
  float currentTime = millis() / 1000.0;  // Time in seconds
  float dt = currentTime - prevTime;
  prevTime = currentTime;

  // ===== Calculate Tilt Angle =====
  gx -= gyroBias;
  gyroChange = gx * dt;
  gyroAngle = gyroAngle + gyroChange;

  accelAngle = -atan2(ay, az) * (180 / PI);

  filteredAngle = Kw * (filteredAngle + gyroChange) + (1 - Kw) * accelAngle;

  // ===== PID Control =====
  error = desiredAngle - filteredAngle;
  proportional = Kp * error;
  integral += error * dt;
  derivative = (error - prevError) / dt;

  prevError = error;

  outputPID = (Kp * proportional) + (Ki * integral) + (Kd * derivative); 

  // ===== Set Motor Speeds =====
  int pwm = constrain(abs(pidOutput), 0, 255);

  if (outputPID > 0) {
    driveMotorsFD(pwm, pwm);   // Move forward to balance forward tilt
  } 
  else {
    driveMotorsFD(-pwm, -pwm); // Move backward to balance backward tilt
  }

  
  // Debugging
  Serial.print("Angle: "); Serial.print(angle);
  Serial.print(" | Output: "); Serial.println(output);
  
  delay(10);  // Small delay to stabilize loop
}

// === Motor Control Functions ===
// Fast Decay PWM
void driveMotorsFD(int speed1, int speed2) {
  // Motor 1 (Left)
  if (speed1 > 0) {   // Drive forward
    analogWrite(Ain1, speed1);
    analogWrite(Ain2, 0);
  } else {            // Drive backward
    analogWrite(Ain1, 0);
    analogWrite(Ain2, abs(speed1));
  }
  // Motor 2 (Right)
  if (speed2 > 0) {   // Drive forward
    analogWrite(Bin1, speed2);
    analogWrite(Bin2, 0);
  } else {            // Drive backward
    analogWrite(Bin1, 0);
    analogWrite(Bin2, abs(speed2));
  }
}

// Slow Decay PWM ()
void driveMotorsSD(int speed1, int speed2) {
  // Motor 1 (Left)
  if (speed1 > 0) {   // Drive forward
    analogWrite(Ain1, 1);
    analogWrite(Ain2, speed1);
  } else {            // Drive backward
    analogWrite(Ain1, abs(speed1));
    analogWrite(Ain2, 1);
  }
  // Motor 2 (Right)
  if (speed2 > 0) {   // Drive forward
    analogWrite(Bin1, 1);
    analogWrite(Bin2, speed2);
  } else {            // Drive backward
    analogWrite(Bin1, abs(speed2));
    analogWrite(Bin2, 1);
  }
}