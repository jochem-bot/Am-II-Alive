#include <math.h>     // Required for ceil()
#include <NeoPixelBus.h>

const uint16_t NUM_LEDS = 80;
const uint8_t LED_PIN = 4;
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_LEDS, LED_PIN);

// Pulse 1 range on strip: 0 to 21 (allow pulseIndex beyond this)
const int PULSE1_START = 0;
const int PULSE1_END = 21;

// Pulse 2 range: 38 to 21 (reversed direction)
const int PULSE2_START = 41;
const int PULSE2_END = 21;

// Pulse 3 range: 59 to 80
const int PULSE3_START = 42;
const int PULSE3_END = 61; // last index is 79 (since LEDs 0..79)

// Extend pulseIndex range beyond strip ends by this margin
const int OVERTRAVEL = 5;

const int tailLength = 1;
const int waitFrames = 10;
const int delayMs = 75;

bool pulseForward = true;
int pulseIndex = 0;
int pauseCounter = 0;
#define MAX_ARRAY_SIZE 200

byte sequenceLength;
byte average_sequence[MAX_ARRAY_SIZE]; // Global: 200 bytes
byte all_sequences[4][MAX_ARRAY_SIZE]; // Global: 4 * 200 = 800 bytes

const int ledPin = 6;           // PWM pin connected to the MOSFET gate
const int maxLevel = 10;        // Max value in the average sequence
const int pwmMax = 230;         // Max PWM value
const int duration = 1500;

float fadeStep = 0.5;
float tailBrightnessScale = 0.2;


unsigned long lastPulseTime = 0;

int playIndex = 0;
unsigned long lastPlayTime = 0;
bool playFinished = false;

#include <math.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

const int SERVO_MID = 300;        // Mid pulse (e.g. ~1500us)
const int SERVO_RANGE_MIN = 60;   // Min amplitude (~1200–1800 µs range)
const int SERVO_RANGE_MAX = 140;  // Max amplitude for more lifelike variation
const int SERVO_BREATH_PIN_1 = 4;
const int SERVO_BREATH_PIN_2 = 5;
const int SERVO_BREATH_PIN_3 = 7;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

unsigned long lastCycleTime = 0;
float breathingPeriod = 5000.0;   // ms, will vary
int breathingAmplitude = 100;     // will vary



void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  pinMode(6, OUTPUT);  // PWM pin for dimming LED (though not actively used for dimming in this snippet)
  pinMode(ledPin, OUTPUT);
  pwm.begin();
  pwm.setPWMFreq(60);  // 60Hz for servos

  pwm.setPWM(SERVO_BREATH_PIN_1, 0, SERVO_MID);
  pwm.setPWM(SERVO_BREATH_PIN_2, 0, SERVO_MID);
  pwm.setPWM(SERVO_BREATH_PIN_3, 0, SERVO_MID);

  randomSeed(analogRead(0));  // Optional: more random on startup
  lastCycleTime = millis();
  chooseNewBreathParameters();
  delay(1300);  
  strip.Begin();
  strip.Show(); 
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, RgbwColor(0, 0, 0, 0));
  }
  strip.Show();

  delay(2000);
}

void generateSequences() {
  byte local_gen_sequence[MAX_ARRAY_SIZE]; 

  // Generate a random sequence length between 100 and 200 (inclusive).
  sequenceLength = random(100, MAX_ARRAY_SIZE + 1);

  // Initialize average_sequence to zeros. It will accumulate sums first.
  for (int i = 0; i < sequenceLength; i++) {
    average_sequence[i] = 0;
  }

  // Generate 4 sequences
  for (int s = 0; s < 4; s++) {
    if (sequenceLength > 0) {
      local_gen_sequence[0] = 0;
    }

    int momentum = 0; // Tracks previous direction (+1 or -1)

    for (int i = 1; i < sequenceLength; i++) {
      byte prevValue = local_gen_sequence[i - 1];

      // Prevent two consecutive 10s before index 100
      if (i < 100 && prevValue == 10) {
        local_gen_sequence[i] = 9;
        momentum = -1;
        continue;
      }

      // Special handling for the last 10 elements: try to reach 10.
      if (sequenceLength - i <= 10) {
        int remainingSteps = sequenceLength - i;
        int distanceTo10 = 10 - prevValue;

        if (remainingSteps >= distanceTo10) {
          local_gen_sequence[i] = min(prevValue + 1, 10);
        } else {
          local_gen_sequence[i] = prevValue;
        }
        continue;
      }

      // Fluctuate around 3
      float centerBias = 0.0;
      if (prevValue < 3) centerBias = 0.3;
      else if (prevValue > 3) centerBias = -0.3;

      // Momentum
      float momentumBias = 0.3 * momentum;

      // Combined bias toward +1
      float bias = 0.5 + centerBias + momentumBias;
      if (bias > 0.9) bias = 0.9;
      if (bias < 0.1) bias = 0.1;

      int delta = (random(0, 100) < (int)(bias * 100)) ? +1 : -1;
      int newValue = prevValue + delta;

      // Clamp between 0 and 10
      if (newValue < 0) newValue = 0;
      if (newValue > 10) newValue = 10;

      momentum = delta;
      local_gen_sequence[i] = (byte)newValue;
    }

    // Ensure last value is 10
    if (sequenceLength > 0 && local_gen_sequence[sequenceLength - 1] != 10) {
      local_gen_sequence[sequenceLength - 1] = 10;
    }

    // Store and accumulate
    for (int i = 0; i < sequenceLength; i++) {
      all_sequences[s][i] = local_gen_sequence[i];
      average_sequence[i] += local_gen_sequence[i];
    }
  }

  // Average over 4 sequences
  for (int i = 0; i < sequenceLength; i++) {
    float avg_float = (float)average_sequence[i] / 4.0;
    average_sequence[i] = (byte)ceil(avg_float);
  }
}


void sendSequences() {

  for (int s = 0; s < 4; s++) {
    Serial.print("Sequence ");
    Serial.print(s + 1);
    Serial.print(" (Length: ");
    Serial.print(sequenceLength);
    Serial.println("):");
    for (int i = 0; i < sequenceLength; i++) {
      Serial.print(all_sequences[s][i]);
      if (i < sequenceLength - 1) {
        Serial.print(",");
      } else {
        Serial.println();
      }
    }
    Serial.println(); // Extra newline after each sequence data
    delay(1000);      // Delay after each sequence is printed
  }
}

void waitForR() {
  Serial.println("Waiting for R from Raspberry Pi...");
  while (true) {
    sendSequences(); // Sends the currently generated set of 4 sequences
    long startTime = millis();
    // Wait up to 5 seconds for 'R'
    while (millis() - startTime < 5000) {
      if (Serial.available() > 0) {
        char received = Serial.read();
        if (received == 'R') {
          Serial.println("R received."); // Optional: confirmation
          return; // Exit waitForR
        }
      }
    }
    // If 'R' not received after 5s, loop continues, sendSequences() is called again
    // with the same set of sequences. This matches original behavior.
  }
}

const int fadeSteps = 20;
int breathState = 0;
unsigned long breathPhaseStart = 0;
int sweepMin = 0, sweepMax = 180;
float sweepPos = 90;
int breathPlayIndex = 0;
bool holdingMax = false;
bool fadingOutAfterHold = false;

int currentPWM = 0;
int targetPWM = 0;
unsigned long holdStartTime = 0;





void updateAll() {


  static int servoAngles[3] = {0, 0, 0};
  static int servoDirections[3] = {1, -1, 1};  // mix directions
  static unsigned long lastServoUpdate = 0;
  const int baseDelay = 200;  // base ms delay at level 0

  // Brightness map (nonlinear)
  const int levelToPercent[11] = {
    0,   // 0 → 0%
    1,   // 1 → 5%
    5,  // 2 → 10%
    10,  // 3 → 20%
    20,  // 4 → 30%
    30,  // 5 → 50%
    40,  // 6 → 70%
    50,  // 7 → 80%
    60,  // 8 → 90%
    70,  // 9 → 95%
    100  // 10 → 100%
  };

  if (playFinished) return;

  if (fadeStep == 0) {
    Serial.print("Now fading to level: ");
    Serial.println(average_sequence[playIndex]);
  }

  if (fadeStep == 0) {
    if (fadingOutAfterHold) {
      targetPWM = 0;
    } else {
      int level = average_sequence[playIndex];
      level = constrain(level, 0, maxLevel);

      // 🔁 Detect double-10 pattern
      if (level == 10 && playIndex + 1 < sequenceLength && average_sequence[playIndex + 1] == 10 && !holdingMax) {
        analogWrite(ledPin, pwmMax);
        currentPWM = pwmMax;
        targetPWM = pwmMax;
        holdingMax = true;
        return;
      }  
      
      

      // Use nonlinear mapping
      int percent = levelToPercent[level];
      targetPWM = map(percent, 0, 100, 0, pwmMax);
    }
  }

  if (holdingMax) {
    if (millis() - holdStartTime < 300000) {
      analogWrite(ledPin, pwmMax);
    }
  }

  float t = float(fadeStep) / (fadeSteps - 1);  // normalized 0..1
  t = t * t * (3 - 2 * t);                      // ease-in-out (smoothstep)
  int interpolatedPWM = currentPWM + (targetPWM - currentPWM) * t;

  // --- LED Pulse Section (unchanged) ---
  RgbwColor off(0, 0, 0, 0);
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, off);
  }

  // Helper lambda to set pixel only if valid index
  auto setPixelSafe = [](int idx, RgbwColor color) {
    if (idx >= 0 && idx < NUM_LEDS) {
      strip.SetPixelColor(idx, color);
    }
  };

  // --- PULSE 1: moving forward 0->21, with overtravel ---
  for (int t = 0; t <= tailLength; t++) {
    int idx = pulseForward ? (pulseIndex - t) : (pulseIndex + t);
    float intensity = 1.0 - (float)t / (tailLength + 1);

    if (idx >= PULSE1_START && idx <= PULSE1_END) {
      uint8_t r, g, b, w;
      if (idx == 0 || idx == 1 || idx == 20 || idx == 21) {
        r = 0; g = 0* intensity; b = 0; w = 0; // green zone
      } else {
        r = 0; g = 250 * intensity; b = 40 * intensity; w = 160 * intensity;
      }
      setPixelSafe(idx, RgbwColor(r, g, b, w));
    }
    // if idx outside LED range, do nothing (off)
  }

  for (int t = 0; t <= tailLength; t++) {
    int idx = pulseForward ? (PULSE2_START - (pulseIndex - t)) : (PULSE2_START - (pulseIndex + t));

    float intensity = 1.0 - (float)t / (tailLength + 1);

    if (idx >= PULSE2_END && idx <= PULSE2_START) {
      RgbwColor color;
      if (idx == 19 || idx == 20 || idx == 39 || idx == 40|| idx == 41) {
        color = RgbwColor(0, 0, 0, 0); // black zone
      } else {
        uint8_t r = 200 * intensity;
        uint8_t g = 20 * intensity;
        uint8_t b = 100 * intensity;
        uint8_t w = 100 * intensity;
        color = RgbwColor(r, g, b, w);
      }
      setPixelSafe(idx, color);
    }
  }


  // --- PULSE 3: moving forward 59->79 with overtravel ---
  for (int t = 0; t <= tailLength; t++) {
    int idx = pulseForward ? (PULSE3_START + (pulseIndex - t)) : (PULSE3_START + (pulseIndex + t));
    float intensity = 1.0 - (float)t / (tailLength + 1);

    if (idx >= PULSE3_START && idx <= PULSE3_END) {
      RgbwColor color;
      if (idx == 62 || idx == 63 || idx == 42 || idx == 43) {
        color = RgbwColor(0, 0, 0, 0); // green zone
      } else {
        uint8_t r = 200 * intensity;
        uint8_t g = 20 * intensity;
        uint8_t b = 100 * intensity;
        uint8_t w = 100 * intensity;
        color = RgbwColor(r, g, b, w);
      }
      setPixelSafe(idx, color);
    }
  }

  strip.Show();

  if (!holdingMax) {
    analogWrite(ledPin, interpolatedPWM);
  } else {
    analogWrite(ledPin, pwmMax);  // Ensure it's steady during the hold
  }

    // --- Breathing-style full 180° sweep servo, slower with 6s cycle ---

  




  fadeStep++;
  if (fadeStep >= fadeSteps) {
    currentPWM = targetPWM;
    fadeStep = 0;

    if (fadingOutAfterHold) {
      playFinished = true;
      fadingOutAfterHold = false;
      return;
    }

    playIndex++;
  }

  // --- Update pulseIndex ---
  int maxPulseSteps = (PULSE1_END - PULSE1_START) + OVERTRAVEL; // extend beyond LED range

  if (pulseForward) {
    if (pulseIndex < maxPulseSteps) {
      pulseIndex++;
    } else if (pauseCounter < waitFrames) {
      pauseCounter++;
    } else {
      pulseForward = false;
      pauseCounter = 0;
    }
  } else {
    if (pulseIndex > 0) {
      pulseIndex--;
    } else if (pauseCounter < waitFrames) {
      pauseCounter++;
    } else {
      pulseForward = true;
      pauseCounter = 0;
    }
  }
  
}




void resetUpdateAllState() {
  holdingMax = false;
  fadingOutAfterHold = false;
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
  generateSequences();
  waitForR();
  delay(60000);  // 1-minute wait

  unsigned long playStartTime = millis();  // ⏱ Reset timer every loop()
  const unsigned long PLAY_TIMEOUT = (4UL * 60UL + 52UL) * 1000UL;  // 4 minutes and 55 seconds;  // 5 minutes
  breathState = 0;
  breathPhaseStart = millis();
  fadeStep = 0;
  currentPWM = 0;
  targetPWM = 0;
  pauseCounter = 0;
  holdingMax = false;

  fadingOutAfterHold = false;

  playFinished = false;
  playIndex = 0;

  while (!playFinished && (millis() - playStartTime) < PLAY_TIMEOUT) {
    updateAll();
    updateBreathingServo();
    delay(75);  // your update rate
  }


  playFinished = true;
  for (int i = 0; i < 11; i++) {
    analogWrite(ledPin, 200 - 20*i);  // Fade from 200 down to 191
    delay(50);  // Adjust delay to control fade speed
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, RgbwColor(0, 0, 0, 0));
  }
  strip.Show();
  delay(15000);  // Cooldown
}
