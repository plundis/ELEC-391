#include <ArduinoBLE.h>

#define BUFFER_SIZE 20

const int Ain1 = 2;  // Ain1 on DRV8833 goes to D2 (pin 2) on arduino
const int Ain2 = 3;  // Ain2 on DRV8833 goes to D3 (pin 3) on arduino
const int Bin2 = 4;  // Bin2 on DRV8833 goes to D4 (pin 4) on arduino
const int Bin1 = 5;  // Bin1 on DRV8833 goes to D5 (pin 5) on arduino

// Define strings to store direction and PWM values
String direction = "Stop";  // Forward, Backward, Left, Right, or Stop
int defaultPWM = 200;     // Default PWM value, can be changed by the user

// Define a custom BLE service and characteristic
BLEService customService("00000000-5EC4-4083-81CD-A10B8D5CFC11");
BLECharacteristic customCharacteristic(
    "00000001-5EC4-4083-81CD-A10B8D5CFC11", BLERead | BLEWrite | BLENotify, BUFFER_SIZE, false);

void setup() {
  Serial.begin(9600);
  delay(3000);  // Short delay for stability
  
  Serial.println("Starting setup...");

  // Initialize motor control pins
  pinMode(Ain1, OUTPUT);
  pinMode(Ain2, OUTPUT);
  pinMode(Bin2, OUTPUT);
  pinMode(Bin1, OUTPUT);
  
  // Initialize the built-in LED to indicate connection status
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Run a quick motor test to verify hardware connections
  //testMotors();

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  // Set the device name and local name
  BLE.setLocalName("Robot-C11");
  BLE.setDeviceName("Robot-C11");

  // Add the characteristic to the service
  customService.addCharacteristic(customCharacteristic);

  // Add the service
  BLE.addService(customService);

  // Set an initial value for the characteristic
  customCharacteristic.writeValue("Waiting for data");

  // Start advertising the service
  BLE.advertise();

  Serial.println("Bluetooth® device active, waiting for connections...");
}

// // Test function to verify motor connections
// void testMotors() {
//   Serial.println("Testing motors...");
  
//   Serial.println("Testing left motor forward");
//   analogWrite(Ain1, 200);
//   analogWrite(Ain2, 0);
//   analogWrite(Bin1, 0);
//   analogWrite(Bin2, 0);
//   delay(1000);
  
//   Serial.println("Testing left motor backward");
//   analogWrite(Ain1, 0);
//   analogWrite(Ain2, 200);
//   analogWrite(Bin1, 0);
//   analogWrite(Bin2, 0);
//   delay(1000);
  
//   Serial.println("Testing right motor forward");
//   analogWrite(Ain1, 0);
//   analogWrite(Ain2, 0);
//   analogWrite(Bin1, 200);
//   analogWrite(Bin2, 0);
//   delay(1000);
  
//   Serial.println("Testing right motor backward");
//   analogWrite(Ain1, 0);
//   analogWrite(Ain2, 0);
//   analogWrite(Bin1, 0);
//   analogWrite(Bin2, 200);
//   delay(1000);
  
//   Serial.println("Stopping all motors");
//   analogWrite(Ain1, 0);
//   analogWrite(Ain2, 0);
//   analogWrite(Bin1, 0);
//   analogWrite(Bin2, 0);
  
//   Serial.println("Motor test complete");
// }

void loop() {
  // Wait for a BLE central to connect
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, HIGH); // Turn on LED to indicate connection

    // Keep running while connected
    while (central.connected()) {
      // Check if the characteristic was written
      if (customCharacteristic.written()) {
        // Get the length of the received data
        int length = customCharacteristic.valueLength();

        // Read the received data
        const unsigned char* receivedData = customCharacteristic.value();

        // Create a properly terminated string
        char receivedString[length + 1]; // +1 for null terminator
        memcpy(receivedString, receivedData, length);
        receivedString[length] = '\0'; // Null-terminate the string

        // Print the received data to the Serial Monitor
        Serial.print("Received data: '");
        Serial.print(receivedString);
        Serial.println("'");

        // Try direct motor control based on commands
        directMotorControl(receivedString);

        // Optionally, respond by updating the characteristic's value
        customCharacteristic.writeValue("Data received");
      }
    }

    digitalWrite(LED_BUILTIN, LOW); // Turn off LED when disconnected
    Serial.println("Disconnected from central.");
  }
}

// Direct motor control function - more robust than string comparison
void directMotorControl(char* command) {
  int pwm = defaultPWM;
  
  // Check for PWM setting command
  if (strncasecmp(command, "PWM", 3) == 0) {
    // Extract the PWM value - Format: "PWM 200"
    char* pwmStr = command + 3; // Skip "PWM"
    while (*pwmStr == ' ') pwmStr++; // Skip spaces
    
    // Convert to integer
    int newPWM = atoi(pwmStr);
    
    // Validate and set the new PWM value
    if (newPWM >= 0 && newPWM <= 255) {
      defaultPWM = newPWM;
      Serial.print("PWM set to: ");
      Serial.println(defaultPWM);
    }
    return;
  }
  
  // Trim the command
  while (*command == ' ') command++;
  
  // Use direct string matching rather than string objects
  // Forward command
  if (strncasecmp(command, "Forward", 7) == 0 || 
      strncasecmp(command, "forward", 7) == 0 ||
      strncasecmp(command, "F", 1) == 0) {
    
    Serial.println("COMMAND RECOGNIZED: Forward");
    direction = "Forward";
    
    // Both motors forward
    analogWrite(Ain1, pwm);
    analogWrite(Ain2, 0);
    analogWrite(Bin1, pwm);
    analogWrite(Bin2, 0);
    
    Serial.print("Forward: Left=");
    Serial.print(pwm);
    Serial.print(", Right=");
    Serial.println(pwm);
  }
  
  // Backward command
  else if (strncasecmp(command, "Backward", 8) == 0 ||
           strncasecmp(command, "backward", 8) == 0 ||
           strncasecmp(command, "Back", 4) == 0 ||
           strncasecmp(command, "back", 4) == 0 ||
           strncasecmp(command, "B", 1) == 0) {
    
    Serial.println("COMMAND RECOGNIZED: Backward");
    direction = "Backward";
    
    // Both motors backward
    analogWrite(Ain1, 0);
    analogWrite(Ain2, pwm);
    analogWrite(Bin1, 0);
    analogWrite(Bin2, pwm);
    
    Serial.print("Backward: Left=");
    Serial.print(pwm);
    Serial.print(", Right=");
    Serial.println(pwm);
  }
  
  // Left command
  else if (strncasecmp(command, "Left", 4) == 0 ||
           strncasecmp(command, "left", 4) == 0 ||
           strncasecmp(command, "L", 1) == 0) {
    
    Serial.println("COMMAND RECOGNIZED: Left");
    direction = "Left";
    
    // Turn left
    analogWrite(Ain1, 0);  // Left motor reverse
    analogWrite(Ain2, pwm/2);
    analogWrite(Bin1, pwm);  // Right motor forward
    analogWrite(Bin2, 0);
    
    Serial.print("Left turn: Left=");
    Serial.print(pwm/2);
    Serial.print(" (reverse), Right=");
    Serial.println(pwm);
  }
  
  // Right command
  else if (strncasecmp(command, "Right", 5) == 0 ||
           strncasecmp(command, "right", 5) == 0 ||
           strncasecmp(command, "R", 1) == 0) {
    
    Serial.println("COMMAND RECOGNIZED: Right");
    direction = "Right";
    
    // Turn right
    analogWrite(Ain1, pwm);  // Left motor forward
    analogWrite(Ain2, 0);
    analogWrite(Bin1, 0);  // Right motor reverse
    analogWrite(Bin2, pwm/2);
    
    Serial.print("Right turn: Left=");
    Serial.print(pwm);
    Serial.print(", Right=");
    Serial.println(pwm/2);
    Serial.println(" (reverse)");
  }
  
  // Stop command
  else if (strncasecmp(command, "Stop", 4) == 0 ||
           strncasecmp(command, "stop", 4) == 0 ||
           strncasecmp(command, "S", 1) == 0) {
    
    Serial.println("COMMAND RECOGNIZED: Stop");
    direction = "Stop";
    
    // Stop all motors
    analogWrite(Ain1, 0);
    analogWrite(Ain2, 0);
    analogWrite(Bin1, 0);
    analogWrite(Bin2, 0);
    
    Serial.println("All motors stopped");
  }
  else {
    Serial.print("Unrecognized command: '");
    Serial.print(command);
    Serial.println("'");
  }
}