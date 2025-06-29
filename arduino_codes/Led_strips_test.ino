#include <NeoPixelBus.h>

const uint16_t NUM_LEDS = 80;
const uint8_t LED_PIN = 3;
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_LEDS, LED_PIN);

// Pulse 1 range on strip: 0 to 21 (allow pulseIndex beyond this)
const int PULSE1_START = 0;
const int PULSE1_END = 21;

// Pulse 2 range: 38 to 21 (reversed direction)
const int PULSE2_START = 41;
const int PULSE2_END = 21;

// Pulse 3 range: 59 to 80
const int PULSE3_START = 60;
const int PULSE3_END = 79; // last index is 79 (since LEDs 0..79)

// Extend pulseIndex range beyond strip ends by this margin
const int OVERTRAVEL = 5;

const int tailLength = 1;
const int waitFrames = 10;
const int delayMs = 75;

bool pulseForward = true;
int pulseIndex = 0;
int pauseCounter = 0;

void setup() {
  strip.Begin();
  strip.Show();
}

void loop() {
  updatePulse();
  delay(delayMs);
}

void updatePulse() {
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
      if (idx == 19 || idx == 20 || idx == 39 || idx == 40) {
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
      if (idx == 59 || idx == 60 || idx == 79 || idx == 80) {
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
