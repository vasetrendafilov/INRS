// Declare variables
float Kp = 7;          // (P)roportional Tuning Parameter
float Ki = 6;          // (I)ntegral Tuning Parameter        
float Kd = 3;          // (D)erivative Tuning Parameter       
float iTerm = 0;       // Used to accumulate error (integral)
float lastTime = 0;    // Records the time the function was last called
float maxPID = 255;    // The maximum value that can be output
float oldValue = 0;    // The last sensor value
float targetValue = 0;
#define LOOP_TIME 10          // Time in ms (10ms = 100Hz)
unsigned long timerValue = 0;
/**
 * PID Controller
 * @param  (target)  The target position/value we are aiming for
 * @param  (current) The current value, as recorded by the sensor
 * @return The output of the controller
 */
void setup() {
 // Put all of your setup code here
  timerValue = millis();
  Serial.begin(115200);
}

/******* MAIN LOOP *********/
void loop() {
  // Only run the controller once the time interval has passed
  if (millis() - timerValue > LOOP_TIME) {
    timerValue = millis();

    // Replace getAngle() with your sensor data reading
    float currentValue = getAngle();

    // Run the PID controller
    float motorOutput = pid(targetValue, currentValue);

    // Replace moveMotors() with your desired output
    Serial.println(motorOutput);
  }
}
float pid(float target, float current) {
  // Calculate the time since function was last called
  float thisTime = millis();
  float dT = thisTime - lastTime;
  lastTime = thisTime;

  // Calculate error between target and current values
  float error = target - current;

  // Calculate the integral term
  iTerm += error * dT; 

  // Calculate the derivative term (using the simplification)
  float dTerm = (oldValue - current) / dT;

  // Set old variable to equal new ones
  oldValue = current;

  // Multiply each term by its constant, and add it all up
  float result = (error * Kp) + (iTerm * Ki) + (dTerm * Kd);

  // Limit PID value to maximum values
  if (result > maxPID) result = maxPID;
  else if (result < -maxPID) result = -maxPID;

  return result;
}
