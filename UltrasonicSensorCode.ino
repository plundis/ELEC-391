const int trigPin = 9;
const int echoPin = 10;
const int speakerPin = 13;

const int moduleFrequency = 1000; // Frequency of the beep
float duration, distance;
unsigned long previousBeepTime = 0;
int beepInterval = 500; // Default beep interval

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // Read distance from ultrasonic sensor at a constant rate
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  Serial.print("Distance: ");
  Serial.println(distance);

  // Map distance to beep interval (closer = faster beeping)
  beepInterval = map((int)distance, 5, 100, 100, 1000);
  beepInterval = constrain(beepInterval, 100, 1000);

  // Check if it's time to beep based on millis()
  unsigned long currentTime = millis();
  if (currentTime - previousBeepTime >= beepInterval) {
    previousBeepTime = currentTime;  // Update the last beep time
    tone(speakerPin, moduleFrequency, 100); // Beep duration fixed at 100ms
  }

  delay(50); // Ensures constant sensor reading rate (~20 readings per second)
}
