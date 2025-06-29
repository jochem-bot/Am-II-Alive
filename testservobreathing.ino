#include <math.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const int SERVO_MID = 300;        // Mid pulse (e.g. ~1500us)
const int SERVO_RANGE_MIN = 60;   // Min amplitude (~1200–1800 µs range)
const int SERVO_RANGE_MAX = 140;  // Max amplitude for more lifelike variation
const int SERVO_BREATH_PIN_1 = 5;
const int SERVO_BREATH_PIN_2 = 7;
const int SERVO_BREATH_PIN_3 = 11;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

unsigned long lastCycleTime = 0;
float breathingPeriod = 5000.0;   // ms, will vary
int breathingAmplitude = 100;     // will vary

void setup() {
  pwm.begin();
  pwm.setPWMFreq(60);  // 60Hz for servos

  pwm.setPWM(SERVO_BREATH_PIN_1, 0, SERVO_MID);
  pwm.setPWM(SERVO_BREATH_PIN_2, 0, SERVO_MID);
  pwm.setPWM(SERVO_BREATH_PIN_3, 0, SERVO_MID);

  randomSeed(analogRead(0));  // Optional: more random on startup
  lastCycleTime = millis();
  chooseNewBreathParameters();
}

void chooseNewBreathParameters() {
  // Randomly choose new period and amplitude
  breathingAmplitude = random(SERVO_RANGE_MIN, SERVO_RANGE_MAX + 1);
  breathingPeriod = random(3000, 5000);  // 4–6.5 seconds per breath
}

void updateBreathingServo() {
  static unsigned long startTime = millis();
  unsigned long currentTime = millis();
  float elapsed = currentTime - startTime;

  float angleSin = sin(2 * M_PI * (elapsed / breathingPeriod));
  int pulse = SERVO_MID + (int)(breathingAmplitude * angleSin);

  pwm.setPWM(SERVO_BREATH_PIN_1, 0, pulse);
  pwm.setPWM(SERVO_BREATH_PIN_2, 0, pulse);
  pwm.setPWM(SERVO_BREATH_PIN_3, 0, pulse);

  // If a full cycle has passed, pick a new amplitude and period
  if ((currentTime - lastCycleTime) > breathingPeriod) {
    lastCycleTime = currentTime;
    startTime = currentTime;  // reset start of sine wave
    chooseNewBreathParameters();
  }
}

void loop() {
  updateBreathingServo();
  delay(75);
}
