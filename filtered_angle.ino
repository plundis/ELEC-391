#include "Arduino_BMI270_BMM150.h"
#include <math.h>  // Include for atan2()

float x, y, z, delta_time;
float gyro_ang = 0.0;
float acc_ang = 0.0;
float filtered_ang = 0.0;
float xangle, yangle, zangle;
float degreesX = 0;
float degreesY = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  delta_time = 1000/IMU.gyroscopeSampleRate();
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Gyroscope in degrees/second");
}

void loop() {

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

  gyro_ang = gyro_ang + x*delta_time/1000;
  }

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

   

    acc_ang = atan2(y,z) * 180 / PI;
  }

  filtered_ang = 0.99*(filtered_ang + gyro_ang) + 0.01*acc_ang;

  //Serial.println(filtered_ang);
  Serial.print("Accelerometer angle: ");
  Serial.print(acc_ang);
  Serial.print("°, Gyroscope angle: ");
  Serial.print(gyro_ang);
  Serial.println("°");

  delay(delta_time/1000);
}
