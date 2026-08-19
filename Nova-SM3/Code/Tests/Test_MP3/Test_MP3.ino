/*
 *   Nova SM3 - Bench Test: DFPlayer Mini MP3 module
 *   TARGET BOARD: Teensy 4.0   (master)
 *
 *   Pins match Nova-SM3_teensy-v5.1 (the version that introduced MP3):
 *     RX = 0, TX = 1   (module RX -> Teensy pin 1, module TX -> Teensy pin 0)
 *     MP3_VOL_PIN = 20 (volume trim pot, analog in)
 *     9600 baud, DFPlayer Mini
 *
 *   *** THIS DIFFERS FROM UPSTREAM ON PURPOSE ***
 *   v5.1 wraps pins 0/1 in SoftwareSerial. On a Teensy 4.0 those pins ARE the
 *   hardware UART (Serial1), and SoftwareSerial there is unreliable. This test
 *   uses Serial1 directly - same wiring, same baud, just the correct peripheral.
 *   If this passes but the main sketch fails, that's the reason.
 *
 *   *** LIBRARY ***
 *   Needs DFRobotDFPlayerMini, which is NOT vendored in ../Arduino Libraries/
 *   (that folder has DFRobot_DF1201S.zip, which is for the DFPlayer Mini PRO).
 *   Install "DFRobotDFPlayerMini" via the IDE Library Manager first.
 *
 *   *** WIRING ***
 *   - DFPlayer is a 3.3V-logic-friendly 5V part. Feed VCC 5V, but put a 1k
 *     resistor in series with its RX line - it is the single most common cause
 *     of noise/no-response on this module.
 *   - SD card must be FAT32, with tracks in /mp3/ named 0001.mp3, 0002.mp3, ...
 *
 *   PASS CRITERIA:
 *     - "DFPlayer online" at boot
 *     - file count > 0 (proves the SD card is readable)
 *     - 'p1' plays audible sound
 */

#include <DFRobotDFPlayerMini.h>

#define MP3_VOL_PIN 20
#define LED_PIN     13

DFRobotDFPlayerMini DFPlayer;

byte online     = 0;
int  sound_vol  = 15;      // 0-30; 25 is upstream's max before small speakers distort
int  sounds_sd  = 0;
byte usePot     = 0;       // set 1 once you've confirmed the pot is wired

unsigned long lastPot = 0;
const unsigned int potInterval = 250;

void showMenu() {
  Serial.println(F("\n--- commands (end with Enter) ---"));
  Serial.println(F("  p<n>   play track n from /mp3 (eg p1)"));
  Serial.println(F("  s      stop"));
  Serial.println(F("  >      next track"));
  Serial.println(F("  <      previous track"));
  Serial.println(F("  v<n>   set volume 0-30 (eg v20)"));
  Serial.println(F("  c      re-read file count from SD"));
  Serial.println(F("  t      toggle live volume-pot tracking (pin 20)"));
  Serial.println(F("  ?      this menu"));
  Serial.println(F("---------------------------------\n"));
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:        Serial.println(F("!! serial timeout - check RX/TX not swapped")); break;
    case WrongStack:     Serial.println(F("!! wrong stack")); break;
    case DFPlayerCardInserted: Serial.println(F("SD inserted")); break;
    case DFPlayerCardRemoved:  Serial.println(F("!! SD REMOVED")); break;
    case DFPlayerPlayFinished:
      Serial.print(F("finished track ")); Serial.println(value); break;
    case DFPlayerError:
      Serial.print(F("!! error: "));
      switch (value) {
        case Busy:         Serial.println(F("card not found")); break;
        case Sleeping:     Serial.println(F("sleeping")); break;
        case FileIndexOut: Serial.println(F("file index out of range")); break;
        case FileMismatch: Serial.println(F("cannot find file")); break;
        default:           Serial.println(value);
      }
      break;
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(MP3_VOL_PIN, INPUT);

  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println(F("\n=== Nova SM3 :: DFPlayer Mini test (Teensy 4.0) ==="));
  Serial.println(F("RX=0  TX=1  (via hardware Serial1)   VOL_POT=20"));

  Serial1.begin(9600);
  delay(100);

  Serial.println(F("\nconnecting (can take ~3s)..."));
  if (DFPlayer.begin(Serial1)) {
    online = 1;
    Serial.println(F("DFPlayer online."));
    digitalWrite(LED_PIN, HIGH);

    DFPlayer.volume(sound_vol);
    DFPlayer.EQ(DFPLAYER_EQ_ROCK);
    delay(50);

    sounds_sd = DFPlayer.readFileCounts();
    Serial.print(F("files on SD: "));
    Serial.println(sounds_sd);
    if (sounds_sd <= 0) {
      Serial.println(F("  ^ 0 or -1 means the SD card was not read."));
      Serial.println(F("    check FAT32 format and /mp3/0001.mp3 naming"));
    }
    Serial.print(F("volume: ")); Serial.println(sound_vol);
  } else {
    Serial.println(F("\nDFPlayer NOT FOUND."));
    Serial.println(F("  - RX/TX swapped? module RX goes to Teensy pin 1"));
    Serial.println(F("  - 1k resistor in series with module RX?"));
    Serial.println(F("  - module needs 5V on VCC, not 3.3V"));
    Serial.println(F("  - SD card inserted and FAT32?"));
  }

  showMenu();
}

void loop() {
  // --- module event stream ---
  if (online && DFPlayer.available()) {
    printDetail(DFPlayer.readType(), DFPlayer.read());
  }

  // --- optional live volume pot ---
  if (usePot && millis() - lastPot > potInterval) {
    lastPot = millis();
    int raw = analogRead(MP3_VOL_PIN);
    int v   = map(raw, 0, 1023, 0, 30);
    if (v != sound_vol) {
      sound_vol = v;
      if (online) DFPlayer.volume(sound_vol);
      Serial.print(F("pot raw:")); Serial.print(raw);
      Serial.print(F("  -> volume:")); Serial.println(sound_vol);
    }
  }

  if (!Serial.available()) return;
  String in = Serial.readStringUntil('\n');
  in.trim();
  if (!in.length()) return;

  char c = in.charAt(0);

  if (in == "?") { showMenu(); return; }

  if (in == "t") {
    usePot = !usePot;
    Serial.println(usePot ? F("pot tracking ON") : F("pot tracking OFF"));
    return;
  }

  if (!online) { Serial.println(F("module offline - fix wiring and reset")); return; }

  if (in == "s") { DFPlayer.stop();     Serial.println(F("stop"));     return; }
  if (in == ">") { DFPlayer.next();     Serial.println(F("next"));     return; }
  if (in == "<") { DFPlayer.previous(); Serial.println(F("previous")); return; }

  if (in == "c") {
    sounds_sd = DFPlayer.readFileCounts();
    Serial.print(F("files on SD: ")); Serial.println(sounds_sd);
    return;
  }

  if (c == 'p') {
    int n = in.substring(1).toInt();
    if (n < 1) { Serial.println(F("track must be >= 1")); return; }
    Serial.print(F("playing /mp3/")); Serial.println(n);
    DFPlayer.playMp3Folder(n);          // same call the main sketch uses
    return;
  }

  if (c == 'v') {
    int v = in.substring(1).toInt();
    if (v < 0 || v > 30) { Serial.println(F("volume must be 0-30")); return; }
    sound_vol = v;
    DFPlayer.volume(sound_vol);
    Serial.print(F("volume: ")); Serial.println(sound_vol);
    return;
  }

  Serial.println(F("unknown command - type ? for menu"));
}
