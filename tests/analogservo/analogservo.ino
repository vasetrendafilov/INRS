// Declare variables
#define feedback  A0   // Analog pin
#define PwmA  9       // pwm 9 in2 7 in1 6 standby 5 
#define AIN1  6
#define AIN2  7
#define STBY 5
float Kp = 1;          // (P)roportional Tuning Parameter
float Ki = 0;          // (I)ntegral Tuning Parameter        
float Kd = 0;          // (D)erivative Tuning Parameter       
float iTerm = 0;       // Used to accumulate error (integral)
float lastTime = 0;    // Records the time the function was last called
float maxPID = 255;    // The maximum value that can be output
float oldValue = 0;    // The last sensor value
float targetValue = 148;
#define LOOP_TIME 10          // Time in ms (10ms = 100Hz)
unsigned long timerValue = 0;
int i = 0;
bool j = false;
/**
 * PID Controller
 * @param  (target)  The target position/value we are aiming for
 * @param  (current) The current value, as recorded by the sensor
 * @return The output of the controller
 */
void setup() {
 // Put all of your setup code here
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  pinMode(STBY,OUTPUT);
  pinMode(PwmA,OUTPUT);
  pinMode(feedback,INPUT);
  timerValue = millis();
  Serial.begin(115200);
  analogWrite(PwmA,0);
  digitalWrite(STBY,HIGH);
}

/******* MAIN LOOP *********/
void loop() {
  // Only run the controller once the time interval has passed
  if (millis() - timerValue > LOOP_TIME) {
    timerValue = millis();

    // Replace getAngle() with your sensor data reading
    float currentValue = float(analogRead(feedback));

    // Run the PID controller
    float motorOutput = pid(targetValue, currentValue);
    if (motorOutput < 0){
      digitalWrite(AIN1,HIGH);
      digitalWrite(AIN2,LOW);
    }else{
      digitalWrite(AIN1,LOW);
      digitalWrite(AIN2,HIGH);
    }
    // Replace moveMotors() with your desired output
    //motorOutput = map(motorOutput, 0, 255, 0, 150);
    //Serial.println(motorOutput);
    Serial.println(analogRead(feedback));
    analogWrite(PwmA,map(abs(int(motorOutput)), 0, 255, 10, 255));

    i++;
    if (i > 500){
      i = 0;
      if (j == true){
        targetValue = 333;
        j=false;
      }else{
        targetValue = 148;
        j=true;
      }
      
    }
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
