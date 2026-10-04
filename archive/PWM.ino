#include <Arduino.h>

const int Ain1 = 2;  // Ain1 on DRV8833 goes to D2 (pin2 2) on arduino
const int Ain2 = 3;  // Ain2 on DRV8833 goes to D3 (pin2 3) on arduino
const int Bin2 = 4;  // Bin2 on DRV8833 goes to D4 (pin2 4) on arduino
const int Bin1 = 5;  // Bin1 on DRV8833 goes to D5 (pin2 5) on arduino

void setup() {
  pinMode(Ain1, OUTPUT);
  pinMode(Ain2, OUTPUT);
  pinMode(Bin2, OUTPUT);
  pinMode(Bin1, OUTPUT);
}

void loop() {
  //both forward 25%
  /*analogWrite(Ain1, 64); // 25% duty cycle (64 out of 255)
  analogWrite(Ain2, 0); // Keep Ain2 low for forward direction
  analogWrite(Bin2, 0); // Keep Bin2 low for forward direction
  analogWrite(Bin1, 64); // 25% duty cycle (64 out of 255)*/

  //both forward 50%
  /*analogWrite(Ain1, 128); // 50% duty cycle (128 out of 255)
  analogWrite(Ain2, 0); // Keep Ain2 low for forward direction
  analogWrite(Bin2, 0); // Keep Bin2 low for forward direction
  analogWrite(Bin1, 128); // 50% duty cycle (128 out of 255)*/

  //both forward 75%
  /*analogWrite(Ain1, 192); // 75% duty cycle (192 out of 255)
  analogWrite(Ain2, 0); // Keep Ain2 low for forward direction
  analogWrite(Bin2, 0); // Keep Bin2 low for forward direction
  analogWrite(Bin1, 192); // 75% duty cycle (192 out of 255)*/

  //both forward 100%
  /*analogWrite(Ain1, 255); // 100% duty cycle (255 out of 255)
  analogWrite(Ain2, 0); // Keep Ain2 low for forward direction
  analogWrite(Bin2, 0); // Keep Bin2 low for forward direction
  analogWrite(Bin1, 255); // 100% duty cycle (255 out of 255)*/

  //both reverse 25%
  /*analogWrite(Ain1, 0); // Keep Ain1 low for reverse direction
  analogWrite(Ain2, 64); // 25% duty cycle (64 out of 255)
  analogWrite(Bin2, 64); // 25% duty cycle (64 out of 255)
  analogWrite(Bin1, 0); // Keep Bin1 low for reverse direction*/

  //both reverse 75%
  /*analogWrite(Ain1, 0); // Keep Ain1 low for reverse direction
  analogWrite(Ain2, 192); // 75% duty cycle (192 out of 255)
  analogWrite(Bin2, 192); // 75% duty cycle (192 out of 255)
  analogWrite(Bin1, 0); // Keep Bin1 low for reverse direction*/

  //opposite direction 1 25%
  /*analogWrite(Ain1, 0); // Keep Ain1 low for opposite direction
  analogWrite(Ain2, 64); // 25% duty cycle (64 out of 255)
  analogWrite(Bin2, 0); // Keep Bin2 low for opposite direction
  analogWrite(Bin1, 64); // 25% duty cycle (64 out of 255)*/

  //opposite direction 1 75%
  /*analogWrite(Ain1, 0); // Keep Ain1 low for opposite direction
  analogWrite(Ain2, 192); // 75% duty cycle (192 out of 255)
  analogWrite(Bin2, 0); // Keep Bin2 low for opposite direction
  analogWrite(Bin1, 192); // 75% duty cycle (192 out of 255)*/

  //opposite direction 2 25%
  analogWrite(Ain1, 64); // Keep Ain1 low for opposite direction
  analogWrite(Ain2, 0); // 25% duty cycle (64 out of 255)
  analogWrite(Bin2, 64); // Keep Bin2 low for opposite direction
  analogWrite(Bin1, 0); // 25% duty cycle (64 out of 255)

  //opposite direction 2 75%
  /*analogWrite(Ain1, 192); // Keep Ain1 low for opposite direction
  analogWrite(Ain2, 0); // 75% duty cycle (192 out of 255)
  analogWrite(Bin2, 192); // Keep Bin2 low for opposite direction
  analogWrite(Bin1, 0); // 75% duty cycle (192 out of 255)*/
}