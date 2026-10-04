#include "Arduino_BMI270_BMM150.h"

float x, y, z, delta_time;
float angle = 0.0;

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

  angle = angle + x*delta_time/1000;

  Serial.print("Gyroscope angle: ");
  Serial.println(angle);
  delay(delta_time/1000);
  }
}
