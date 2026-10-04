#include "Arduino_BMI270_BMM150.h"
#include <ArduinoBLE.h>
#include <math.h>
#include <Arduino.h>

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID            "55de00bf-711c-4a8d-b332-8ccf014f7733"
#define CONTROL_CHAR_UUID       "d63d52fe-8eb5-4257-8557-578923bb0eb5" // ESP32 → Arduino (joystick control)
#define TELEMETRY_CHAR_UUID     "25d60f40-f380-4c69-8e2b-ad35ddabeb57" // Arduino → ESP32 (tilt angle)
#define SENSOR_CHAR_UUID        "1fb9a9cd-a7ae-4d1d-a333-a6253d848a00" // Arduino → ESP32 (ultrasonic sensor)

// Ultrasonic sensor pins
const int trigPin1 = 9;
const int echoPin1 = 10;
const int trigPin2 = 8;
const int echoPin2 = 7;
const int speakerPin = 6;

// Debug options
#define DEBUG_PRINT false         // Set to true to enable debug printing
#define DEBUG_SERIAL Serial      // Serial port to use for debug output

// Print and send intervals
#define ANGLE_PRINT_INTERVAL 500  // Print angle every 500ms
#define ANGLE_SEND_INTERVAL 250   // Send angle every 250ms
#define DISTANCE_SEND_INTERVAL 350 // Increased to 350ms
#define STATUS_LINE_LENGTH 80     // Length of status line for formatting
#define SONAR_PRINT_INTERVAL 350  // Print sonar data every 350ms

// Create a BLE service and characteristics
BLEService balanceService(SERVICE_UUID);
BLECharacteristic dataCharacteristic(CONTROL_CHAR_UUID, BLERead | BLEWrite | BLENotify, 2); // 2 bytes for control packet
BLECharacteristic angleCharacteristic(TELEMETRY_CHAR_UUID, BLERead | BLENotify, 4); // 4 bytes for float angle
BLECharacteristic ultrasonicCharacteristic(SENSOR_CHAR_UUID, BLERead | BLENotify, 8); // 8 bytes for three float distances

// Packet structure to match ESP32
struct ControlPacket {
  uint8_t button;       // Button state (0 or 1)
  uint8_t direction;    // Direction code (0 - 8)
};

struct DistancePacket {
  float distance1;
  float distance2;
};

// Direction mapping for debug output
const char* directionNames[] = {
  "Center (Stop)",
  "Forward",
  "Right (Grad F)",
  "Right (In-Place)",
  "Right (Grad B)",
  "Backward",
  "Left (Grad B)",
  "Left (In-Place)",
  "Left (Grad F)"
};

// Store last received joystick command
uint8_t lastButtonState = 0;
uint8_t lastDirection = 0;

// Motor Driver Pins
const int Ain1 = 2;  
const int Ain2 = 3;  
const int Bin2 = 4;  
const int Bin1 = 5;  

// PID Gains (Tune These)
float Kp = 8.0;  //8
float Ki = 95.0;  //45
float Kd = 0.35;  //0.9

// PID Parameters
float leftTurnPWM, rightTurnPWM = 0.0;
bool limit = false;
float desiredAngle = 0.0;  // Target balance angle
float proportional;
float integral = 0.0;
float derivative;
float lastDerivative = 0.0;
float currentError;
float prevError = 0.0;
float dt;
unsigned long currentTime;
unsigned long prevTime = 0;
float PIDoutput;
float pwm;
float dutyCycle;

// Complementary Filter Parameters
float Kw = 0.99;  // Complementary filter weight
float filteredAngle;
float accelAngle = 0.0;
float gyroAngle = 0.0;
float gyroChange;
float gyroBias = 0.0;

// Ultrasonic sensor variables
const int moduleFrequency = 1000; // Frequency of the beep
float duration1 = 0, distance1 = 0;  // Initialize to 0 instead of 400
float duration2 = 0, distance2 = 0;  // Initialize to 0 instead of 400
unsigned long previousBeepTime = 0;
int beepInterval = 500; // Default beep interval
// Add these variables near the ultrasonic sensor variables section:
bool validReading1 = false;  // Flag to indicate valid reading from sensor 1
bool validReading2 = false;  // Flag to indicate valid reading from sensor 2

// Non-blocking sonar variables
unsigned long lastSonarTrigger1 = 0;
unsigned long lastSonarTrigger2 = 0;
unsigned long startEcho1Time = 0;
unsigned long startEcho2Time = 0;
const unsigned long SONAR_TRIGGER_INTERVAL = 200; // Reduced from 450ms to 200ms for faster updates
const unsigned long SONAR_TIMEOUT = 25000; // Timeout for echo (in microseconds, ~4m range)

// BLE connection stabilization time
unsigned long connectionStabilizationTime = 0;
const unsigned long CONNECTION_STABILIZATION_PERIOD = 2000; // 2 seconds

// BLE connection status
bool bleConnected = false;
// BLE initialization flag
bool bleInitialized = false;

// Serial output variables
unsigned long lastAnglePrintTime = 0;
unsigned long lastAngleSendTime = 0;
unsigned long lastDistanceSendTime = 0;
unsigned long lastDirectionPrintTime = 0;
unsigned long lastSonarPrintTime = 0;
bool printSeparator = false;

enum SonarState {
  IDLE, 
  TRIGGER_SENSOR_1,
  WAIT_ECHO_1,
  WAIT_ECHO_1_END,
  TRIGGER_SENSOR_2,
  WAIT_ECHO_2,
  WAIT_ECHO_2_END
};
SonarState sonarState = IDLE;
unsigned long lastSonarStateChange = 0;

// Debug print helper function
void debugPrint(const char* message) {
  if (DEBUG_PRINT) {
    DEBUG_SERIAL.println(message);
  }
}

// Print sonar data only
void printSonarData() {
  if (!DEBUG_PRINT) return;
  
  DEBUG_SERIAL.print("SONAR: Dist1=");
  DEBUG_SERIAL.print(distance1, 1);
  DEBUG_SERIAL.print("cm, Dist2=");
  DEBUG_SERIAL.print(distance2, 1);
  DEBUG_SERIAL.println("cm");
}

// Print a separator line for better serial readability
void printSeparatorLine() {
  if (!DEBUG_PRINT) return;
  
  DEBUG_SERIAL.print("+");
  for (int i = 0; i < STATUS_LINE_LENGTH - 2; i++) {
    DEBUG_SERIAL.print("-");
  }
  DEBUG_SERIAL.println("+");
}

// Print joystick direction
void printJoystickDirection(uint8_t direction, uint8_t buttonState) {
  if (!DEBUG_PRINT) return;
  
  printSeparatorLine();
  DEBUG_SERIAL.print("| JOYSTICK: ");
  if (direction < 9) {
    DEBUG_SERIAL.print(directionNames[direction]);
    
    // Pad with spaces for alignment
    int padLength = 20 - strlen(directionNames[direction]);
    for (int i = 0; i < padLength; i++) {
      DEBUG_SERIAL.print(" ");
    }
  } else {
    DEBUG_SERIAL.print("UNKNOWN (");
    DEBUG_SERIAL.print(direction);
    DEBUG_SERIAL.print(")");
  }
  
  DEBUG_SERIAL.print(" | BUTTON: ");
  DEBUG_SERIAL.print(buttonState ? "PRESSED" : "RELEASED");
  
  // Pad to end of line
  int buttonTextLen = buttonState ? 7 : 8;
  int padLength = STATUS_LINE_LENGTH - 42 - buttonTextLen;
  for (int i = 0; i < padLength; i++) {
    DEBUG_SERIAL.print(" ");
  }
  DEBUG_SERIAL.println("|");
  printSeparatorLine();
}

// Print angle only
void printRobotStatus(float angle) {
  if (!DEBUG_PRINT) return;
  
  printSeparatorLine();
  
  DEBUG_SERIAL.print("| ANGLE: ");
  // Print angle with 2 decimal places
  if (angle < 0) {
    DEBUG_SERIAL.print('-');
    angle = -angle;
  } else {
    DEBUG_SERIAL.print(' ');
  }
  int wholePart = (int)angle;
  int decimalPart = (int)((angle - wholePart) * 100);
  
  // Right-align the number with spaces
  if (wholePart < 10) DEBUG_SERIAL.print(' ');
  DEBUG_SERIAL.print(wholePart);
  DEBUG_SERIAL.print('.');
  if (decimalPart < 10) DEBUG_SERIAL.print('0');
  DEBUG_SERIAL.print(decimalPart);
  
  DEBUG_SERIAL.print(" | DIST1: ");
  DEBUG_SERIAL.print(distance1, 1);
  DEBUG_SERIAL.print("cm | DIST2: ");
  DEBUG_SERIAL.print(distance2, 1);
  DEBUG_SERIAL.print("cm");
  
  // Pad to end of line
  int padLength = STATUS_LINE_LENGTH - 50; // Adjusted for extra content
  for (int i = 0; i < padLength; i++) {
    DEBUG_SERIAL.print(" ");
  }
  DEBUG_SERIAL.println("|");
  
  printSeparatorLine();
}

// Send the angle to ESP32 via BLE
void sendAngleToBLE(float angle) {
  if (!bleConnected) return;
  
  // Convert float to byte array using a union
  union {
    float angleValue;
    uint8_t bytes[4];
  } angleConverter;
  
  angleConverter.angleValue = angle;
  
  // Send to ESP32
  angleCharacteristic.writeValue(angleConverter.bytes, 4);
  
  // Print debug info
  if (DEBUG_PRINT) {
    DEBUG_SERIAL.print("A");  // Just print a short indicator instead of full message
  }
}

// Send distance data to ESP32 via BLE
void sendDistanceToBLE(float dist1, float dist2) {
  if (!bleConnected) return;
  
  // Use the distance packet structure
  DistancePacket distPacket;
  
  // Set the distance values
  distPacket.distance1 = dist1;
  distPacket.distance2 = dist2;
  
  // Send to ESP32
  ultrasonicCharacteristic.writeValue((uint8_t*)&distPacket, sizeof(distPacket));
  
  // Print debug info
  if (DEBUG_PRINT) {
    DEBUG_SERIAL.print("D(");
    DEBUG_SERIAL.print(distPacket.distance1, 1);
    DEBUG_SERIAL.print(",");
    DEBUG_SERIAL.print(distPacket.distance2, 1);
    DEBUG_SERIAL.print(")");
  }
}

// Trigger sonar pulse for the specified sensor
void triggerSonarPulse(int trigPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
}

// Fast, non-blocking ultrasonic state machine
//---- Replace the existing updateSonar() function with this improved version ----

// Replace the updateSonar function with this BLE-friendly version

void updateSonar() {
  unsigned long currentMillis = millis();
  unsigned long currentMicros = micros();
  
  // Only update sonar in appropriate intervals
  if (currentMillis - lastSonarStateChange < 5) {
    return; // Don't process sonar too frequently
  }
  
  switch (sonarState) {
    case IDLE:
      // Only start a new sonar cycle after the interval
      if (currentMillis - lastSonarTrigger1 >= SONAR_TRIGGER_INTERVAL) {
        sonarState = TRIGGER_SENSOR_1;
        lastSonarStateChange = currentMillis;
      }
      break;
      
    case TRIGGER_SENSOR_1:
      // Trigger first sensor
      triggerSonarPulse(trigPin1);
      startEcho1Time = currentMicros;
      lastSonarTrigger1 = currentMillis;
      sonarState = WAIT_ECHO_1;
      lastSonarStateChange = currentMillis;
      break;
      
    case WAIT_ECHO_1:
      // Check if echo pulse started
      if (digitalRead(echoPin1) == HIGH) {
        // Echo pulse started, record the start time
        startEcho1Time = currentMicros;
        // Move to next state to wait for echo end
        sonarState = WAIT_ECHO_1_END;
        lastSonarStateChange = currentMillis;
      } else if (currentMicros - startEcho1Time > SONAR_TIMEOUT) {
        // Timeout waiting for echo to start
        if (DEBUG_PRINT) {
          DEBUG_SERIAL.println("S1: No echo");
        }
        sonarState = TRIGGER_SENSOR_2;
        lastSonarStateChange = currentMillis;
      }
      break;
      
    case WAIT_ECHO_1_END:
      // Wait for echo to end
      if (digitalRead(echoPin1) == LOW) {
        // Echo pulse ended, calculate duration
        duration1 = currentMicros - startEcho1Time;
        if (duration1 > 0 && duration1 < SONAR_TIMEOUT) {
          distance1 = (duration1 * 0.0343) / 2;
          if (distance1 > 400) distance1 = 400;
          if (DEBUG_PRINT) {
            DEBUG_SERIAL.print("S1:");
            DEBUG_SERIAL.print(distance1, 1);
            DEBUG_SERIAL.print(" ");
          }
        }
        sonarState = TRIGGER_SENSOR_2;
        lastSonarStateChange = currentMillis;
      } else if (currentMicros - startEcho1Time > SONAR_TIMEOUT) {
        // Timeout waiting for echo to end
        sonarState = TRIGGER_SENSOR_2;
        lastSonarStateChange = currentMillis;
      }
      break;
      
    case TRIGGER_SENSOR_2:
      // Add delay between sensors to avoid interference
      if (currentMillis - lastSonarTrigger1 >= 50) {
        // Trigger second sensor
        triggerSonarPulse(trigPin2);
        startEcho2Time = currentMicros;
        lastSonarTrigger2 = currentMillis;
        sonarState = WAIT_ECHO_2;
        lastSonarStateChange = currentMillis;
      }
      break;
      
    case WAIT_ECHO_2:
      // Check if echo pulse started
      if (digitalRead(echoPin2) == HIGH) {
        // Echo pulse started, record the start time
        startEcho2Time = currentMicros;
        // Move to next state to wait for echo end
        sonarState = WAIT_ECHO_2_END;
        lastSonarStateChange = currentMillis;
      } else if (currentMicros - startEcho2Time > SONAR_TIMEOUT) {
        // Timeout waiting for echo to start
        if (DEBUG_PRINT) {
          DEBUG_SERIAL.println("S2: No echo");
        }
        sonarState = IDLE;
        lastSonarStateChange = currentMillis;
      }
      break;
      
    case WAIT_ECHO_2_END:
      // Wait for echo to end
      if (digitalRead(echoPin2) == LOW) {
        // Echo pulse ended, calculate duration
        duration2 = currentMicros - startEcho2Time;
        if (duration2 > 0 && duration2 < SONAR_TIMEOUT) {
          distance2 = (duration2 * 0.0343) / 2;
          if (distance2 > 400) distance2 = 400;
          if (DEBUG_PRINT) {
            DEBUG_SERIAL.print("S2:");
            DEBUG_SERIAL.print(distance2, 1);
            DEBUG_SERIAL.print(" ");
          }
        }
        sonarState = IDLE;
        lastSonarStateChange = currentMillis;
      } else if (currentMicros - startEcho2Time > SONAR_TIMEOUT) {
        // Timeout waiting for echo to end
        sonarState = IDLE;
        lastSonarStateChange = currentMillis;
      }
      break;
  }
  
  // Debug output sonar readings at regular intervals
  if (currentMillis - lastSonarPrintTime >= SONAR_PRINT_INTERVAL) {
    lastSonarPrintTime = currentMillis;
    printSonarData();
  }
}
// Manage beeper with non-blocking approach
void updateBeep() {
  unsigned long currentMillis = millis();
  
  // Update beeping logic based on nearest distance
  float minDistance = min(distance1, distance2);
  beepInterval = map(constrain((int)minDistance, 5, 100), 5, 100, 100, 1000);
  
  // Non-blocking beep
  if (currentMillis - previousBeepTime >= beepInterval) {
    previousBeepTime = currentMillis;
    tone(speakerPin, moduleFrequency, 50); // Shorter beep duration (50ms)
  }
  
  // Print sonar data at regular intervals
  if (currentMillis - lastSonarPrintTime >= SONAR_PRINT_INTERVAL) {
    lastSonarPrintTime = currentMillis;
    printSonarData();
  }
}

void setup() {
  // Initialize Serial for debugging
  if (DEBUG_PRINT) {
    DEBUG_SERIAL.begin(115200);
    delay(100); // Short delay for serial to initialize
    DEBUG_SERIAL.println("\n\n=== BalanceBot Starting ===");
  }
  
  // Initialize LED for status indication
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn on LED to indicate startup
  
  // Initialize ultrasonic sensor pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(speakerPin, OUTPUT);
  
  // Set default pin states
  digitalWrite(trigPin1, LOW);
  digitalWrite(trigPin2, LOW);
  
  // BLE initialization flag
  bleInitialized = false;

  // Initialize IMU without blocking on failure
  if (!IMU.begin()) {
    debugPrint("IMU initialization failed!");
    // Signal IMU error with LED pattern: 3 quick blinks
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
    // Continue anyway - don't block the program
  } else {
    debugPrint("IMU initialized successfully");
  }

  // Initialize BLE without blocking on failure
  bleInitialized = BLE.begin();
  if (!bleInitialized) {
    debugPrint("BLE initialization failed!");
    // Signal BLE error with LED pattern: 2 quick blinks
    for (int i = 0; i < 2; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(300);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
    }
    // Continue anyway - we'll balance even without BLE
  } else {
    debugPrint("BLE initialized successfully");
  }

  // Gyroscope calibration with LED indicator
  debugPrint("Starting gyro calibration...");
  calibrateGyro();
  debugPrint("Gyro calibration complete!");

  // Set motor pins as outputs
  pinMode(Ain1, OUTPUT);
  pinMode(Ain2, OUTPUT);
  pinMode(Bin1, OUTPUT);
  pinMode(Bin2, OUTPUT);

  // Force motors to stop at startup
  driveMotorsSD(0, 0);

  // Set up BLE if it initialized successfully
  if (bleInitialized) {
    // Set advertised local name and service UUID
    BLE.setLocalName("BalanceBot");
    BLE.setAdvertisedServiceUuid(SERVICE_UUID);

    // Add the characteristics to the service
    balanceService.addCharacteristic(dataCharacteristic);
    balanceService.addCharacteristic(angleCharacteristic);
    balanceService.addCharacteristic(ultrasonicCharacteristic);

    // Add service to BLE
    BLE.addService(balanceService);

    // Set initial values for the characteristics
    uint8_t initialControlValues[2] = {0, 0};
    dataCharacteristic.writeValue(initialControlValues, 2);
    
    // Initial angle value
    union {
      float angleValue;
      uint8_t bytes[4];
    } angleConverter;
    angleConverter.angleValue = 0.0;
    angleCharacteristic.writeValue(angleConverter.bytes, 4);
    
    // Initial distance values
    DistancePacket initialDist;
    initialDist.distance1 = 400.0;
    initialDist.distance2 = 400.0;
    ultrasonicCharacteristic.writeValue((uint8_t*)&initialDist, sizeof(initialDist));

    // Start advertising
    BLE.setAdvertisingInterval(100);  // Faster advertising (100ms)
    BLE.advertise();
    debugPrint("BLE advertising started");
  }

  prevTime = micros();
  lastAnglePrintTime = millis();
  lastAngleSendTime = millis();
  lastDistanceSendTime = millis();
  lastDirectionPrintTime = 0;  // Initialize to 0 to print first update immediately
  lastSonarPrintTime = 0;      // Initialize sonar print time
  
  // Turn off LED to indicate setup complete
  digitalWrite(LED_BUILTIN, LOW);
  debugPrint("Setup complete, entering main loop");
}

void calibrateGyro() {
  float x, y, z;
  int samples = 0;
  gyroBias = 0;
  
  // LED on during calibration
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Fast blink during calibration
  while (samples < 1000) {
    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(x, y, z);
      gyroBias += x;
      samples++;
      
      // Blink LED every 100 samples as visual indicator
      if (samples % 100 == 0) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        if (DEBUG_PRINT && samples % 200 == 0) {
          DEBUG_SERIAL.print("Calibration progress: ");
          DEBUG_SERIAL.print(samples);
          DEBUG_SERIAL.println("/1000");
        }
      }
    }
  }
  gyroBias /= samples;
  
  // Calibration complete - LED off
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

void loop() {
  // Only check for BLE connections if BLE is initialized
  BLEDevice central;
  if (bleInitialized) {
    central = BLE.central();
  }
  
  // If a central is connected to peripheral
  if (bleInitialized && central) {
    if (!bleConnected) {
      // Flash LED to show BLE connection
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
      }
      bleConnected = true;
      connectionStabilizationTime = millis(); // Set stabilization time when connected
      debugPrint("Connected to central");
    }
    
    // While the central is still connected
    while (central.connected()) {      
      // Check for incoming BLE commands
      if (dataCharacteristic.written()) {
        // Read the full 2-byte packet
        uint8_t buffer[2];
        dataCharacteristic.readValue(buffer, 2);
        
        // Parse the packet
        uint8_t buttonState = buffer[0];
        uint8_t direction = buffer[1];
        
        // Only print if direction or button state changed
        if (direction != lastDirection || buttonState != lastButtonState) {
          lastDirection = direction;
          lastButtonState = buttonState;
          
          // Print joystick direction for debugging
          printJoystickDirection(direction, buttonState);
          lastDirectionPrintTime = millis();
        }
        
        // Handle joystick direction for robot movement
        handleJoystickCommand(direction);
      }

      // Run balance control
      balanceControl();

      // Update sonar readings (light operation)
      updateSonar();
  
      // Update beep based on sonar readings (light operation)
      updateBeep();
      
      // Check if it's time to send angle data to ESP32
      unsigned long currentMillis = millis();
      if (currentMillis - lastAngleSendTime > ANGLE_SEND_INTERVAL) {
        lastAngleSendTime = currentMillis;
        sendAngleToBLE(filteredAngle);
      }
      
      // Send distance data at appropriate intervals
      // Only send when sonar state machine is idle (measurement complete)
      if (currentMillis - lastDistanceSendTime > DISTANCE_SEND_INTERVAL && 
          sonarState == IDLE && 
          currentMillis - lastSonarStateChange > 20) { // Add a small buffer time
        lastDistanceSendTime = currentMillis;
        
        // Constrain distance values to valid range before sending
        float safeDistance1 = (distance1 > 0 && distance1 < 400) ? distance1 : 400;
        float safeDistance2 = (distance2 > 0 && distance2 < 400) ? distance2 : 400;
        
        // Send the distance values
        sendDistanceToBLE(safeDistance1, safeDistance2);
        
        if (DEBUG_PRINT) {
          DEBUG_SERIAL.print(" [STATE: IDLE] "); // Debug which state we're in when sending
        }
      }
    }
    
    // When the central disconnects
    bleConnected = false;
    lastDirection = 0; // Reset direction when disconnected
    debugPrint("Disconnected from central");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
  } 
  else {
    // No BLE connection, still balance and use ultrasonic
    balanceControl();
    updateSonar();
    updateBeep();
  }
}

void balanceControl() {
  currentTime = micros();
  dt = (currentTime - prevTime) / 1E6;  // Time step in seconds
  prevTime = currentTime;
  
  // Skip if time step is unreasonable (startup or overflow)
  if (dt > 0.1 || dt <= 0) return;
  
  // Check for stabilization period only if we're connected via BLE
  // This allows the robot to balance even without BLE connection
  unsigned long currentMillis = millis();
  if (bleConnected && currentMillis - connectionStabilizationTime < CONNECTION_STABILIZATION_PERIOD) {
    driveMotorsSD(0, 0);  // Force motors to stop
    integral = 0;         // Reset integral term to prevent windup
    return;               // Skip the rest of balance control
  }
  
  // IMU Data
  float ax, ay, az, gx, gy, gz;
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);
  } else {
    // Skip if no IMU data available
    return;
  }
  
  // Tilt Calculation
  gx -= gyroBias;
  gyroChange = gx * dt;
  gyroAngle += gyroChange;
  accelAngle = -atan2(ay, az) * (180.0 / PI) + 0.95;
  filteredAngle = Kw * (filteredAngle + gyroChange) + (1 - Kw) * accelAngle;
  
  // PID Control
  currentError = desiredAngle - filteredAngle;
  proportional = Kp * currentError;
  integral += currentError * dt;
  
  // Anti-windup protection - uncommented for stability
  integral = constrain(integral, -10.0, 10.0);
  
  derivative = (currentError - prevError) / dt;
  derivative = 0.8 * derivative + 0.2 * lastDerivative; // Low-pass filter
  lastDerivative = derivative;
  prevError = currentError;
  
  PIDoutput = proportional + (Ki * integral) + (Kd * derivative);
  
  // Periodically print angle information
  unsigned long currentPrintMillis = millis();
  
  // Print angle stats if:
  // 1. It's been ANGLE_PRINT_INTERVAL milliseconds since the last print AND
  // 2. It's been at least 1000ms since the last joystick direction print OR no joystick prints yet
  if (currentPrintMillis - lastAnglePrintTime > ANGLE_PRINT_INTERVAL && 
      (currentPrintMillis - lastDirectionPrintTime > 1000 || lastDirectionPrintTime == 0)) {
    lastAnglePrintTime = currentPrintMillis;
    printRobotStatus(filteredAngle);
  }
  
  // Dead Zone Logic
  const float deadZone = 0.0;  // ±1° dead zone
  const float killZone = 25.0; // Robot can't balance past this point
  if (abs(currentError) < deadZone) {  
    pwm = 0;  // No movement near balance point
    driveMotorsSD(0, 0);
  } else if (abs(filteredAngle) >= killZone) {
      pwm = 0;  // Kill motors if robot tilts too far
      driveMotorsSD(0, 0);
  } else {
    // Drive forward/backward based on error direction
    if (PIDoutput < 0) {
      pwm = 255 - constrain(abs(PIDoutput - 0), 0, 255);
      driveMotorsSD(pwm + leftTurnPWM, pwm + rightTurnPWM);   // Forward
    } else {
      pwm = 255 - constrain(abs(PIDoutput + 0), 0, 255);
      driveMotorsSD(-pwm, -pwm);  // Backward
    }
  }
}

void handleJoystickCommand(uint8_t direction) {
  // Handle joystick direction for robot movement
  switch(direction) {
    case 0: // Center - Balance using PID
      // Do nothing here - the PID loop will handle balancing
      desiredAngle = 0.0;
      leftTurnPWM = 0.0;
      rightTurnPWM = 0.0;
      break;
    case 1: // Up - Forward
      desiredAngle = 0.7;
      leftTurnPWM = 10.0;
      rightTurnPWM = 10.0;
      break;
    case 2: // Up-Right - Forward-Right
      desiredAngle = 0.0;
      leftTurnPWM = 20.0;
      rightTurnPWM = -10.0;
      break;
    case 3: // Right - Turn Right
      desiredAngle = 0.0;
      leftTurnPWM = 20.0;
      rightTurnPWM = -10.0;
      break;
    case 4: // Down-Right - Backward-Right
      desiredAngle = 0.0;
      leftTurnPWM = 20.0;
      rightTurnPWM = -10.0;
      break;
    case 5: // Down - Backward
      desiredAngle = -0.7;
      leftTurnPWM = -10.0;
      rightTurnPWM = -10.0;
      break;
    case 6: // Down-Left - Backward-Left
      desiredAngle = 0.0;
      leftTurnPWM = -10.0;
      rightTurnPWM = 15.0;
      break;
    case 7: // Left - Turn Left
      desiredAngle = 0.0;
      leftTurnPWM = -10.0;
      rightTurnPWM = 15.0;
      break;
    case 8: // Up-Left - Forward-Left
      desiredAngle = 0.0;
      leftTurnPWM = -10.0;
      rightTurnPWM = 15.0;
      break;
  }
}

// ===== SLOW DECAY PWM =====
void driveMotorsSD(int speedL, int speedR) {
  // Left Motor
  if (speedL > 0) {   // Forward (Slow Decay)
    analogWrite(Ain1, speedL);  
    analogWrite(Ain2, 255);
  } else {            // Reverse (Slow Decay)
    analogWrite(Ain1, 255);
    analogWrite(Ain2, abs(speedL));
  }
  // Right Motor
  if (speedR > 0) {   // Forward (Slow Decay)
    analogWrite(Bin1, speedR);  
    analogWrite(Bin2, 255);
  } else {            // Reverse (Slow Decay)
    analogWrite(Bin1, 255);
    analogWrite(Bin2, abs(speedR));
  }
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