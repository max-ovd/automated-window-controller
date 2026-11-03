/* ACTUATOR CONSTANTS */
constexpr uint8_t EN   = 4; // Enable pin (L_EN and R_EN tied together)
constexpr uint8_t RPWM = 5;
constexpr uint8_t LPWM = 6;
constexpr uint8_t POWER_LEVEL = 100; // X/255 PWM Signal
constexpr unsigned long ACTUATION_DURATION = 10000; // 10s in ms
constexpr unsigned long ACTUATION_DELAY = 600000; // 10 min in ms

/* ARDUINO CONSTANTS */
constexpr uint8_t SENSOR_PIN = A7;
constexpr uint8_t LED_PIN    = 9;
constexpr uint8_t SIZE       = 10;

constexpr float CALIBRATION_FACTOR = 0.0; 
constexpr float INPUT_VOLTAGE      = 5.08; 
constexpr unsigned long MEASUREMENT_DELAY = 60000; // 1 minute in ms

/* GLOBAL VARIABLES */
float temps[SIZE];
uint8_t tempIndex = 0;
float tempSum = 0.0;
unsigned long currentMillis = 0;
unsigned long lastMeasurementTime = 0;

/* Actuator States */
enum ActuatorState {
  STOPPED,
  OPENING,
  CLOSING
};
ActuatorState actuatorState = STOPPED;
unsigned long lastActuationTime = 0;

/* FUNCTION DECLARATIONS */
void updateActuator();
void stopActuator();

void addSample(float num);
float getAverage();

void setup() {
  Serial.begin(9600);
  
  pinMode(EN, OUTPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);

  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  // Enable H-bridge
  digitalWrite(EN, HIGH);

  // Ensure motor starts at rest
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);

  // Initialize temperature array with default temp of 22
  for (uint8_t i = 0; i < SIZE; i++) {
    temps[i] = 22.0;
  }
  tempSum = 22.0 * SIZE; // Pre-calculate starting sum
}

void loop() {
  currentMillis = millis();
  updateActuator();

  // Non-blocking timer check
  if (currentMillis - lastMeasurementTime >= MEASUREMENT_DELAY) {
    lastMeasurementTime = currentMillis;

    /* Convert LM35DZ voltage reading --> temperature reading */
    int reading = analogRead(SENSOR_PIN);
    float voltageReading = (reading / 1024.0) * INPUT_VOLTAGE;
    float currentTemp = (voltageReading * 100.0) - CALIBRATION_FACTOR;

    addSample(currentTemp);
    float averageTemp = getAverage();

    Serial.print("Current Temp: ");
    Serial.print(currentTemp);
    Serial.print(" C | Avg Temp: ");
    Serial.println(averageTemp);

    if (currentMillis - lastActuationTime >= ACTUATION_DELAY && actuatorState == STOPPED) {
      if (averageTemp > 23.0) {
        actuatorState = OPENING;
        lastActuationTime = currentMillis;
      }
      else if (averageTemp < 21.0) {
        lastActuationTime = currentMillis;
        actuatorState = CLOSING;
      }
    }
  }
}

/* FUNCTION DECLARATIONS */
void updateActuator() {
  if (actuatorState == STOPPED)
    return;
  
  if (currentMillis - lastActuationTime >= ACTUATION_DURATION) {
    actuatorState = STOPPED;
    stopActuator();
    return;
  }

  switch (actuatorState) {
    case OPENING: 
      analogWrite(RPWM, POWER_LEVEL);
      analogWrite(LPWM, 0);
      break;

    case CLOSING:
      analogWrite(RPWM, 0);
      analogWrite(LPWM, POWER_LEVEL);
      break;
    
    default:
      break;
  }
}

void stopActuator() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}

void addSample(float new_temp) {
  tempSum -= temps[tempIndex];
  temps[tempIndex] = new_temp;
  tempSum += new_temp;

  tempIndex = (tempIndex + 1) % SIZE; // Advance index circularly
}

float getAverage() {
  return tempSum / SIZE;
}