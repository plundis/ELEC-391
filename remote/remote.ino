#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
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

// Direction and button state for LCD
String lastDirection = "";
int lastButtonState = -1;

struct ControlPacket {
  uint8_t button;       // Button state (0 or 1)
  uint8_t direction;    // Direction code (0 - 8)
};

// The remote service and characteristic UUIDs we want to connect to
static BLEUUID serviceUUID("19B10000-E8F2-537E-4F6C-D104768A1214");
static BLEUUID charUUID("19B10001-E8F2-537E-4F6C-D104768A1214");

// Connection variables
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Callback function for notifications
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify) {
  //Serial.print("Notification received: ");
  //Serial.println(*pData, DEC);
}

// Connect to the BLE Server
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected");
  }
};

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer() {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  
  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic
  if(pRemoteCharacteristic->canRead()) {
    // Fix: Use String instead of std::string
    String valueStr = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value is: ");
    Serial.println(valueStr);
  }

  // Register for notifications if possible
  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, check if it contains the service we are looking for
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
  }
};

void setup() {
  Serial.begin(9600);
  Serial.println("ESP32 BLE Client");

  // Initialize joystick button press
  pinMode(SW_PIN, INPUT_PULLUP);

  // Initialize LCD
  lcd.begin(16,2);
  lcd.clear();

  // Initialize the BLE environment
  BLEDevice::init("ESP32Client");

  // Retrieve a Scanner and set the callback
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  int xValue = analogRead(VRX_PIN); 
  int yValue = analogRead(VRY_PIN); 
  int buttonState = (digitalRead(SW_PIN) == LOW) ? 1 : 0;
  String direction = "Center (Stop)";  // Default to Center
  uint8_t directionCode = 0;           // Default to Center
  float receivedAngle;
  uint8_t receiveBuffer[4];

  // Apply dead zone
  if (abs(xValue - X_CENTER) < DEADZONE) xValue = X_CENTER;
  if (abs(yValue - Y_CENTER) < DEADZONE) yValue = Y_CENTER;

  // Determine movement
  if (yValue < Y_CENTER) {  
    if (xValue < X_CENTER) {
      direction = "Left (Grad F)";
      directionCode = 8;
    } else if (xValue > X_CENTER) {
      direction = "Right (Grad F)";
      directionCode = 2;
    } else {
      direction = "Forward";
      directionCode = 1;
    }
  } 
  else if (yValue > Y_CENTER) {  
    if (xValue < X_CENTER) {
      direction = "Left (Grad B)";
      directionCode = 6;
    }
    else if (xValue > X_CENTER) {
      direction = "Right (Grad B)";
      directionCode = 4;
    }
    else {
      direction = "Backward";
      directionCode = 5;
    }
  } 
  else { 
    if (xValue < X_CENTER) {
      direction = "Left (In-Place)";
      directionCode = 7;
    }
    else if (xValue > X_CENTER) { 
      direction = "Right (In-Place)";
      directionCode = 3;
    }
  }

  // Display direction on LCD
  if (buttonState != lastButtonState || direction != lastDirection) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(buttonState ? "Button Pressed" : direction);
    
    lastDirection = direction;
    lastButtonState = buttonState;
  } 

  // BLE connection logic
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server.");
    }
    doConnect = false;
  }

  // BLE communication
  // BLE communication
  if (connected) {
    static unsigned long lastTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastTime > 200) {
      lastTime = currentTime;

      // Send packet
      uint8_t sendBuffer[2] = {buttonState, directionCode};
      pRemoteCharacteristic->writeValue(sendBuffer, sizeof(sendBuffer), true);

      // Wait a bit for Arduino to process and respond
      delay(10);

      // Receive the float angle (4 bytes)
      if(pRemoteCharacteristic->canRead()) {
        // Read the value as a raw byte array
        String valueStr = pRemoteCharacteristic->readValue();
        
        Serial.print("Received data length: ");
        Serial.println(valueStr.length());
        
        // We need exactly 4 bytes for a float
        if(valueStr.length() == 4) {
          // Create a union to safely convert between bytes and float
          union {
            float angle_value;
            uint8_t bytes[4];
          } converter;
          
          // Copy the bytes from the received data
          for(int i = 0; i < 4; i++) {
            converter.bytes[i] = (uint8_t)valueStr[i];
          }
          
          // Now converter.angle_value contains the float
          float receivedAngle = converter.angle_value;
          
          // Print the raw bytes for debugging
          Serial.print("Bytes: ");
          for(int i = 0; i < 4; i++) {
            Serial.print(converter.bytes[i]); 
            Serial.print(" ");
          }
          Serial.println();
          
          // Safety check for NaN before using the value
          if (!isnan(receivedAngle) && isfinite(receivedAngle)) {
            Serial.print("Received angle: ");
            Serial.println(receivedAngle, 2);
            
            // Update LCD with angle
            lcd.setCursor(0, 1);
            lcd.print("Angle: ");
            lcd.print(receivedAngle, 2);
            lcd.print("   ");
          } else {
            Serial.println("Invalid angle value received");
          }
        } 
      }
    }
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }
}
