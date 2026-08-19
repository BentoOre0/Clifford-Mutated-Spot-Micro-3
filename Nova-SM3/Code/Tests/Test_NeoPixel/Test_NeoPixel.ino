/*
 *   Nova SM3 - Bench Test: NeoPixel "eyes" + brightness trim pot
 *   TARGET BOARD: Arduino Nano   (slave)
 *
 *   Pins match Nova-SM3_nano-v5.1:
 *     LED_EYES_PIN    = 2    (4 pixels, NEO_GRB + NEO_KHZ800)
 *     LED_BRIGHT_PIN  = A3   (brightness trim pot, analog in)
 *
 *   *** WIRING NOTE ***
 *   WS2812s want 5V data. A Nano drives that fine. If pixels flicker or show
 *   wrong colours, the usual causes are: no common ground between the LED
 *   supply and the Nano, no ~330R in series on the data line, or no 1000uF
 *   cap across the LED supply.
 *
 *   PASS CRITERIA:
 *     - the walk test lights pixels 0,1,2,3 IN ORDER (proves index mapping)
 *     - R/G/B test shows true red, green, blue - if red and green swap, the
 *       strip is RGB not GRB, change NEO_GRB below
 *     - turning the pot smoothly changes brightness
 */

#include <Adafruit_NeoPixel.h>

#define LED_EYES_PIN   2
#define LED_EYES_NUM   4
#define LED_BRIGHT_PIN A3

Adafruit_NeoPixel led_eyes = Adafruit_NeoPixel(LED_EYES_NUM, LED_EYES_PIN, NEO_GRB + NEO_KHZ800);

int rgb_bright = 80;      // matches the slave sketch default
byte usePot    = 1;

int stage = 0;
unsigned long lastStage = 0;
const unsigned int stageInterval = 4000;

unsigned long lastStep = 0;
int step = 0;

void allOff() {
  for (int i = 0; i < LED_EYES_NUM; i++) led_eyes.setPixelColor(i, 0, 0, 0);
  led_eyes.show();
}

void setup() {
  pinMode(LED_BRIGHT_PIN, INPUT);

  Serial.begin(115200);
  delay(200);

  Serial.println(F("\n=== Nova SM3 :: NeoPixel test (Arduino Nano) ==="));
  Serial.print(F("data pin=")); Serial.print(LED_EYES_PIN);
  Serial.print(F("  pixels=")); Serial.print(LED_EYES_NUM);
  Serial.print(F("  brightness pot=A")); Serial.println(3);
  Serial.println();

  led_eyes.begin();
  led_eyes.setBrightness(rgb_bright);
  allOff();

  Serial.println(F("stage 1: walk - pixels light 0,1,2,3 in order"));
}

void loop() {
  // --- live brightness from trim pot ---
  if (usePot) {
    static int lastB = -1;
    int raw = analogRead(LED_BRIGHT_PIN);
    int b   = map(raw, 0, 1023, 5, 255);
    if (abs(b - lastB) > 4) {          // hysteresis, pots are noisy
      lastB = b;
      rgb_bright = b;
      led_eyes.setBrightness(rgb_bright);
      led_eyes.show();
      Serial.print(F("pot raw:")); Serial.print(raw);
      Serial.print(F("  -> brightness:")); Serial.println(rgb_bright);
    }
  }

  // --- stage advance ---
  if (millis() - lastStage > stageInterval) {
    lastStage = millis();
    stage = (stage + 1) % 4;
    step = 0;
    allOff();
    switch (stage) {
      case 0: Serial.println(F("stage 1: walk - pixels light 0,1,2,3 in order")); break;
      case 1: Serial.println(F("stage 2: R / G / B - check colours are TRUE")); break;
      case 2: Serial.println(F("stage 3: all white - check for dead pixels")); break;
      case 3: Serial.println(F("stage 4: rainbow fade")); break;
    }
  }

  if (millis() - lastStep < 120) return;
  lastStep = millis();

  switch (stage) {

    case 0: {   // walk one pixel at a time
      allOff();
      int p = step % LED_EYES_NUM;
      led_eyes.setPixelColor(p, 255, 255, 255);
      led_eyes.show();
      if (step % LED_EYES_NUM == 0 && step) Serial.println(F("  ...wrapped"));
      step++;
      break;
    }

    case 1: {   // primaries, all pixels
      int phase = (step / 6) % 3;
      for (int i = 0; i < LED_EYES_NUM; i++) {
        led_eyes.setPixelColor(i,
          phase == 0 ? 255 : 0,
          phase == 1 ? 255 : 0,
          phase == 2 ? 255 : 0);
      }
      led_eyes.show();
      step++;
      break;
    }

    case 2: {   // solid white
      for (int i = 0; i < LED_EYES_NUM; i++) led_eyes.setPixelColor(i, 255, 255, 255);
      led_eyes.show();
      break;
    }

    case 3: {   // rainbow
      for (int i = 0; i < LED_EYES_NUM; i++) {
        led_eyes.setPixelColor(i, led_eyes.ColorHSV(
          (uint16_t)(((step * 512) + (i * 65536L / LED_EYES_NUM)) % 65536)));
      }
      led_eyes.show();
      step++;
      break;
    }
  }
}
