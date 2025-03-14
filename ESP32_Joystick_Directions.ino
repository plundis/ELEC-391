#define VRX_PIN 34  // X-axis (ADC1_6)
#define VRY_PIN 35  // Y-axis (ADC1_7)
#define SW_PIN  32  // Joystick button (digital)

#define X_CENTER 2770
#define Y_CENTER 2730
#define THRESHOLD 400  // Define a range to consider movement (adjustable)

// Direction states
#define DIR_CENTER  0
#define DIR_RIGHT   1
#define DIR_LEFT    2
#define DIR_FORWARD 4
#define DIR_BACK    8

void setup() {
    Serial.begin(9600);
    pinMode(SW_PIN, INPUT_PULLUP); // Use pull-up to detect button press

    // Give some time for the serial connection to establish
    delay(500);
    Serial.println("Joystick control initialized");
}

void loop() {
    // Read joystick values
    int xValue = analogRead(VRX_PIN); // Read X-axis
    int yValue = analogRead(VRY_PIN); // Read Y-axis
    int buttonState = digitalRead(SW_PIN); // Read button (LOW when pressed)
    
    // Calculate X and Y offsets from center
    int xOffset = xValue - X_CENTER;
    int yOffset = yValue - Y_CENTER;
    
    // Determine direction flags for X and Y axes
    int directionState = DIR_CENTER;
    
    // Check X-axis (left/right)
    if (xOffset < -THRESHOLD) {
        directionState |= DIR_LEFT;
    } else if (xOffset > THRESHOLD) {
        directionState |= DIR_RIGHT;
    }
    
    // Check Y-axis (forward/backward)
    if (yOffset < -THRESHOLD) {
        directionState |= DIR_FORWARD;
    } else if (yOffset > THRESHOLD) {
        directionState |= DIR_BACK;
    }
    
    // Map the direction state to a readable string
    String direction = getDirectionString(directionState);
    
    // Print direction, button state, and raw joystick values
    Serial.print("Direction: "); 
    Serial.print(direction);
    Serial.print(" | Button: "); 
    Serial.print(buttonState == LOW ? "PRESSED" : "RELEASED");
    Serial.print(" | VRX: "); 
    Serial.print(xValue);
    Serial.print(" | VRY: "); 
    Serial.println(yValue);
    
    delay(100); // Small delay to prevent flooding the serial port
}

String getDirectionString(int dirState) {
    // Convert the direction state to a readable string
    if (dirState == DIR_CENTER) {
        return "center";
    }
    
    String result = "";
    
    // Check for forward/backward component
    if (dirState & DIR_FORWARD) {
        result += "forward";
    } else if (dirState & DIR_BACK) {
        result += "backward";
    }
    
    // Add hyphen if we have both components
    if ((dirState & (DIR_FORWARD | DIR_BACK)) && (dirState & (DIR_LEFT | DIR_RIGHT))) {
        result += "-";
    }
    
    // Check for left/right component
    if (dirState & DIR_LEFT) {
        result += "left";
    } else if (dirState & DIR_RIGHT) {
        result += "right";
    }
    
    return result;
}
