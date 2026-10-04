#include "Arduino_BMI270_BMM150.h"
#include <math.h>  // Include for atan2()

float x, y, z, delta_time;
float gyro_ang = 0.0;
float acc_ang = 0.0;
float filtered_ang = 0.0;
float prev_filtered_ang = 0.0;
float k = 0.8;
float gyro_change;
float gyro_bias = 0.0;  // Variable to store gyroscope bias

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  delta_time = 1.0 / IMU.gyroscopeSampleRate();
  // Serial.print("Gyroscope sample rate = ");
  // Serial.print(IMU.gyroscopeSampleRate());
  // Serial.println(" Hz");
  // Serial.println();
  // Serial.println("Gyroscope in degrees/second");

  // Optional: Calculate gyroscope bias over time to subtract it from the readings
  for (int i = 0; i < 1000; i++) {
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(x, y, z);
      gyro_bias += x;  // Accumulate the x-axis gyroscope values
      delay(10);  // Wait to gather enough samples
    }
  }
  gyro_bias /= 1000.0;  // Average the bias value
  // Serial.print("Gyroscope bias: ");
  // Serial.println(gyro_bias);
}

void loop() {

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    // Apply bias correction to the gyroscope x-axis value
    x -= gyro_bias;

    gyro_change = x * delta_time;
    gyro_ang = gyro_ang + gyro_change;
  }

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    // Calculate the accelerometer angle using atan2
    acc_ang = -atan2(y, z) * 180 / PI;
  }

  filtered_ang = k * (filtered_ang + gyro_change) + (1 - k) * acc_ang;

  // Print the values
  Serial.print("Filtered angle: ");
  Serial.print(filtered_ang);
  Serial.print("°, Accelerometer angle: ");
  Serial.print(acc_ang);
  Serial.print("°, Gyroscope angle: ");
  Serial.print(gyro_ang);
  Serial.println("°");
}
