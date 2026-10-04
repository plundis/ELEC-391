#include <LiquidCrystal.h>

// LCD pins
#define RS  18
#define EN  21
#define DB4 4
#define DB5 16
#define DB6 17
#define DB7 5

// Assign parameters for 4-bit initialization
LiquidCrystal lcd(RS, EN, DB4, DB5, DB6, DB7);

// Joystick pins
#define VRX_PIN 34  // X-axis
#define VRY_PIN 35  // Y-axis
#define SW_PIN  32  // Joystick button

// Joystick calibration
#define X_CENTER 1846  
#define Y_CENTER 1816  
#define DEADZONE 200  // Ignore small movements to avoid drift

void setup() {
    // Initialize serial
    Serial.begin(9600);
    pinMode(SW_PIN, INPUT_PULLUP);

    // Initialize LCD
    lcd.begin(16,2);
    lcd.clear();
}

void loop() {
    int xValue = analogRead(VRX_PIN); 
    int yValue = analogRead(VRY_PIN); 
    int buttonState = digitalRead(SW_PIN); 


    // Apply dead zone
    if (abs(xValue - X_CENTER) < DEADZONE) xValue = X_CENTER;
    if (abs(yValue - Y_CENTER) < DEADZONE) yValue = Y_CENTER;

    String direction = "Center (Stop)";

    // Determine movement
    if (yValue < Y_CENTER) {  
        if (xValue < X_CENTER) direction = "Left (Grad F)";
        else if (xValue > X_CENTER) direction = "Right (Grad F)";
        else direction = "Forward";
    } 
    else if (yValue > Y_CENTER) {  
        if (xValue < X_CENTER) direction = "Left (Grad B)";
        else if (xValue > X_CENTER) direction = "Right (Grad B)";
        else direction = "Backward";
    } 
    else { 
        if (xValue < X_CENTER) direction = "Left (In-Place)";
        else if (xValue > X_CENTER) direction = "Right (In-Place)";
    }

    // Display direction on LCD
    if(buttonState == LOW) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Button Pressed");
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Direction:");
      lcd.setCursor(0, 1);
      lcd.print(direction);
    }
    
    // Print direction
    Serial.print("Direction: "); Serial.print(direction);
    Serial.print(" | X: "); Serial.print(xValue);
    Serial.print(" | Y: "); Serial.print(yValue);
    Serial.print(" | Button: "); Serial.println(buttonState == LOW ? "PRESSED" : "RELEASED");

    delay(100);
}
