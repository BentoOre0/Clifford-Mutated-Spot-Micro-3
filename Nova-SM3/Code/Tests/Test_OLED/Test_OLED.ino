/*
 *   Nova SM3 - Bench Test: SSD1331 96x64 RGB OLED
 *   TARGET BOARD: Arduino Nano   (slave)
 *
 *   Pins match Nova-SM3_nano-v5.1:
 *     SCLK = 13   MOSI = 11   CS = 10   DC = 9   RST = 8   (hardware SPI)
 *     OLED_ACTIVE_PIN = 12  (panel button, digital in)
 *
 *   *** SILKSCREEN WARNING ***
 *   SSD1331 breakouts label these inconsistently. Map by function, not name:
 *     SCL / SCK / D0  -> 13
 *     SDA / MOSI / D1 -> 11
 *     CS              -> 10
 *     DC / A0 / RS    ->  9
 *     RES / RST       ->  8
 *   Getting DC and RST swapped gives a lit-but-blank screen, which is the
 *   single most common failure here.
 *
 *   PASS CRITERIA:
 *     - colour bars appear in the RIGHT order (red green blue at top)
 *       wrong order = RGB/BGR panel variant, not a wiring fault
 *     - text is legible and not mirrored/offset
 *     - no dead rows or columns
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>

int sclk = 13;   //scl or sck
int mosi = 11;   //sda or mosi
int cs   = 10;
int dc   =  9;
int rst  =  8;

#define SCREEN_WIDTH  96
#define SCREEN_HEIGHT 64
#define OLED_ACTIVE_PIN 12

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);

#define WHITE   0xFFFF
#define GRAY    0xAAAA
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0

int stage = 0;
unsigned long lastStage = 0;
const unsigned int stageInterval = 3000;

void stageColourBars() {
  display.fillScreen(BLACK);
  const uint16_t cols[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE, GRAY};
  int h = SCREEN_HEIGHT / 8;
  for (int i = 0; i < 8; i++) {
    display.fillRect(0, i * h, SCREEN_WIDTH, h, cols[i]);
  }
  Serial.println(F("stage 1: colour bars (R G B Y C M W grey, top to bottom)"));
}

void stageText() {
  display.fillScreen(BLACK);
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.println(F("NOVA SM3"));
  display.setTextColor(GREEN);
  display.println(F("OLED test"));
  display.setTextColor(CYAN);
  display.print(F("96x64 "));
  display.println(F("SSD1331"));
  display.setTextColor(YELLOW);
  display.println(F("0123456789"));
  display.setTextColor(MAGENTA);
  display.println(F("abcdefghij"));
  display.setTextColor(RED);
  display.print(F("up "));
  display.print(millis() / 1000);
  display.println(F("s"));
  Serial.println(F("stage 2: text - check legible, not mirrored"));
}

void stageEdges() {
  // full-extent frame: proves no rows/cols are cut off or offset
  display.fillScreen(BLACK);
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.drawRect(1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2, RED);
  display.drawLine(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, GREEN);
  display.drawLine(SCREEN_WIDTH - 1, 0, 0, SCREEN_HEIGHT - 1, GREEN);
  display.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 12, BLUE);
  Serial.println(F("stage 3: frame + diagonals - all 4 edges must be visible"));
}

void stageGradient() {
  display.fillScreen(BLACK);
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    uint8_t v = map(x, 0, SCREEN_WIDTH - 1, 0, 31);
    display.drawFastVLine(x, 0,  21, v << 11);          // red ramp
    display.drawFastVLine(x, 21, 21, (v * 2) << 5);     // green ramp (6 bits)
    display.drawFastVLine(x, 42, 22, v);                // blue ramp
  }
  Serial.println(F("stage 4: gradients - should be smooth, no banding/tearing"));
}

void setup() {
  pinMode(OLED_ACTIVE_PIN, INPUT);

  Serial.begin(115200);
  delay(200);

  Serial.println(F("\n=== Nova SM3 :: SSD1331 OLED test (Arduino Nano) ==="));
  Serial.print(F("SCLK=")); Serial.print(sclk);
  Serial.print(F("  MOSI=")); Serial.print(mosi);
  Serial.print(F("  CS=")); Serial.print(cs);
  Serial.print(F("  DC=")); Serial.print(dc);
  Serial.print(F("  RST=")); Serial.println(rst);
  Serial.println(F("\nIf the screen stays black: check DC and RST are not swapped.\n"));

  display.begin();
  display.fillScreen(BLACK);

  stageColourBars();
  lastStage = millis();
}

void loop() {
  // panel button state, reported on change
  static int lastBtn = -1;
  int btn = digitalRead(OLED_ACTIVE_PIN);
  if (btn != lastBtn) {
    lastBtn = btn;
    Serial.print(F("panel button (pin 12): "));
    Serial.println(btn ? F("HIGH") : F("LOW"));
  }

  if (millis() - lastStage < stageInterval) return;
  lastStage = millis();

  stage = (stage + 1) % 4;
  switch (stage) {
    case 0: stageColourBars(); break;
    case 1: stageText();       break;
    case 2: stageEdges();      break;
    case 3: stageGradient();   break;
  }
}
