/*
 *   NovaSM3 - a Spot-Mini Micro clone
 *   Version: 6.0
 *   Version Date: 2026-08-19
 *
 *   TARGET BOARD: Arduino Nano  --  this is the SLAVE controller.
 *   Its partner is Nova-SM3_teensy-v6.0, the master that does all the
 *   thinking: servos, IMU, remote, sensors and serial commands.
 *
 *   Original Author:  Chris Locke - cguweb@gmail.com
 *   Nova's website:  https://novaspotmicro.com
 *   GitHub Project:  https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3
 *   YouTube Playlist:  https://www.youtube.com/watch?v=00PkTcGWPvo&list=PLcOZNHwM_I2a3YZKf8FtUjJneKGXCfduk
 *
 *   Version 6.0 is a readability pass over version 5.1 by Jeremy, with
 *   Claude Code. Behaviour is unchanged. See README.md for the full list.
 *
 *
 *   WHAT THIS BOARD DOES
 *   --------------------
 *   The Nano owns only the peripherals wired directly to it:
 *
 *     - the SSD1331 SPI colour OLED
 *     - four NeoPixel "eyes"
 *     - two ultrasonic distance sensors
 *     - a few bytes of EEPROM holding its own settings
 *
 *   It makes no decisions. The Teensy sends single-byte commands over i2c and
 *   this board carries them out. Every command byte is named and documented in
 *   NovaSlaveProtocol.h, which is shared with the master.
 *
 *
 *   HOW IT RUNS
 *   -----------
 *   Two i2c interrupt handlers and one non-blocking loop:
 *
 *     receiveEvent()    stores the incoming byte in `req`
 *     requestCallback() acts on `req` and returns a two-byte response
 *     loop()            advances the LED animation and the display on their
 *                       own millis() timers, and watches the OLED button
 *
 *   The one piece of state that changes what a byte means is `serial_oled`:
 *   0 = the byte is a system command, 1 = it is a display command. The byte
 *   'X' toggles it. See NovaSlaveProtocol.h for the full explanation.
 *
 *
 *   HOW THIS SKETCH IS ORGANISED
 *   ----------------------------
 *     Nova-SM3_nano-v6.0.ino   globals, setup(), loop(), i2c handlers  <- start here
 *     NovaSlaveProtocol.h      the command bytes - keep in sync with the master's copy
 *     NovaBitmap.h             the boot logo
 *     Display.ino              everything drawn on the OLED
 *     Leds.ino                 the NeoPixel eye animations
 *     Settings.ino             EEPROM settings and the self-reset
 *
 *
 *   MEMORY
 *   ------
 *   The Nano is nearly full. Before adding anything, check the compiler's
 *   final report - there is very little flash headroom left.
 *
 *   RELEASE NOTES (v5.1, carried over)
 *      Added saving / retrieving eeprom data to enable changing settings from the master
 *      Replaced i2c oled with SPI oled
 *      Added NewPing for ultrasonic sensors
 *      Added command switches for active settings
 *      Added battery gauge display code
 *      Added radar target display code
*/

//set Nova SM3 version
#define VERSION 6.0

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <EEPROM.h>
#include <NewPing.h>

//the command bytes this board understands, shared with the master.
//IMPORTANT: this file must stay identical to the master's copy.
#include "NovaSlaveProtocol.h"
#include "NovaBitmap.h"


/*
   -------------------------------------------------------
   Settings
    :these are DEFAULTS ONLY. load_ep_data() overwrites all five from EEPROM
    :during setup, so changing a value here has no effect on a board that has
    :already been run - it will read back whatever EEPROM holds.
    :
    :The master changes splash_active remotely with the reboot commands, which
    :is what makes the setting persist across a power cycle.
   -------------------------------------------------------
*/
byte debug1 = 0;                  //print i2c traffic and display activity to serial
byte rgb_active = 1;              //NeoPixel eyes fitted
byte oled_active = 1;             //OLED display fitted
byte uss_active = 1;              //ultrasonic sensors fitted
byte splash_active = 1;           //play the boot animation


/*
   -------------------------------------------------------
   Pin map - Arduino Nano (slave)
   -------------------------------------------------------

     pin  | used for
     -----+---------------------------------------------------
      2   | NeoPixel eyes data
      3   | self-reset, wired back to this board's own RESET
      4   | right ultrasonic ECHO
      5   | right ultrasonic TRIGGER
      6   | left ultrasonic ECHO
      7   | left ultrasonic TRIGGER
      8   | OLED reset
      9   | OLED data/command select
      10  | OLED chip select
      11  | OLED data      (hardware SPI MOSI)
      12  | OLED on/off button
      13  | OLED clock     (hardware SPI SCK)
      A3  | eye brightness potentiometer
      A4  | i2c SDA to the master
      A5  | i2c SCL to the master
*/

//i2c link to the master
#define SLAVE_ID 1                //this board's i2c address
#define RESET_PIN 3               //pulled LOW to reboot this board on command
byte serial_oled = 0;             //0 = incoming bytes are system commands, 1 = display commands
int resp;                         //value returned to the master on the next request
char req;                         //most recent command byte received


/*
   -------------------------------------------------------
   OLED display (SSD1331, hardware SPI)
    :SPI clock and data are fixed by the hardware to pins 13 and 11, so only
    :the three control lines are configurable here
   -------------------------------------------------------
*/
#define OLED_CS 10
#define OLED_DC 9
#define OLED_RST 8
#define SCREEN_WIDTH 96
#define SCREEN_HEIGHT 64
Adafruit_SSD1331 display = Adafruit_SSD1331(OLED_CS, OLED_DC, OLED_RST);

//16-bit RGB565 colours
#define WHITE 0xFFFF
#define GRAY 0xAAAA
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0

unsigned long lastOLEDUpdate = 0;
unsigned int oledInterval = 250;
unsigned long lastOLEDClear = 0;
unsigned long oledClearInterval = 5000;   //blank the screen this long after the last command
int oled_command = 0;                     //screen waiting to be drawn, 0 = nothing pending

//front panel button that blanks the display, and the eye brightness pot
#define OLED_ACTIVE_PIN 12
#define LED_BRIGHT_PIN A3


/*
   -------------------------------------------------------
   NeoPixel eyes
    :four pixels: 0 and 1 are the left eye, 2 and 3 the right
   -------------------------------------------------------
*/
#define LED_EYES_PIN 2
#define LED_EYES_NUM 4
Adafruit_NeoPixel led_eyes = Adafruit_NeoPixel(LED_EYES_NUM, LED_EYES_PIN, NEO_GRB + NEO_KHZ800);

unsigned int rgbInterval = 30;    //ms between animation steps while idle
unsigned long lastRGBUpdate = 0;
int rgb_bright = 80;              //0-100, read from the brightness pot each pass
int rgb_del = 256;                //countdown used by the colour flow pattern
int current_led = 0;              //which pixel the wipe patterns are on
int fadeStep = 0;                 //progress through the current fade, 0 = restart it
int fade_steps = 400;             //how many steps one fade takes

int pattern = 0;                  //which animation is running, set by the master
int pattern_int = 400;            //ms between steps while a pattern is running
int pattern_cnt = 3;              //repeats left before the eyes go idle
int pattern_step = 0;             //sub-step within one blink
int pattern_side = 0;             //which eye the next colour command applies to
int cur_rgb_val1[3] = {55, 0, 200};  //left eye colour, r/g/b
int cur_rgb_val2[3] = {55, 0, 200};  //right eye colour, r/g/b


/*
   -------------------------------------------------------
   Ultrasonic distance sensors
   -------------------------------------------------------
*/
#define SONAR_NUM 2
#define L_TRIGPIN 7
#define L_ECHOPIN 6
#define R_TRIGPIN 5
#define R_ECHOPIN 4
#define MAX_DISTANCE 300          //cm; anything further away is ignored
NewPing sonar[SONAR_NUM] = {
  NewPing(L_TRIGPIN, L_ECHOPIN, MAX_DISTANCE),
  NewPing(R_TRIGPIN, R_ECHOPIN, MAX_DISTANCE),
};
#define SONAR_LEFT 0
#define SONAR_RIGHT 1
int dist_l;                       //last good left reading, in cm
int dist_r;                       //last good right reading, in cm


/*
   -------------------------------------------------------
   Battery gauge
    :the master works out which level the pack has dropped to and sends the
    :level number; this board only draws it
   -------------------------------------------------------
*/
int blevel_id = 0;                        //0 = healthiest, 9 = critical
float batt_levels[10] = {                 //voltage each level represents
   11.2, 11.1, 11.0, 10.9, 10.8,
   10.7, 10.6, 10.5, 10.4, 10.3,
};


/*
   -------------------------------------------------------
   EEPROM layout
    :one byte per setting. Read back by load_ep_data() at boot, which is why
    :the defaults above only apply to a board that has never been written.
   -------------------------------------------------------
*/
#define EEPROM_DEBUG1         0
#define EEPROM_RGB_ACTIVE     1
#define EEPROM_OLED_ACTIVE    2
#define EEPROM_USS_ACTIVE     3
#define EEPROM_SPLASH_ACTIVE  4

void load_ep_data();
void reset_slave();


/*
   -------------------------------------------------------
   Setup
    :load settings, bring up the peripherals, play the boot splash

    The splash is skipped when splash_active is clear, which the master sets
    remotely with the quiet reboot command. It matters because the master
    WAITS for this board during its own boot - see command_slave() there - so
    a long splash makes the whole robot slower to start.
   -------------------------------------------------------
*/
void setup() {
  Serial.begin(19200);
  delay(200);

  load_ep_data();
  delay(500);

  if (debug1) {
    Serial.println("eeprom loaded:");
    Serial.print("debug1: ");Serial.println(debug1);
    Serial.print("rgb_active: ");Serial.println(rgb_active);
    Serial.print("oled_active: ");Serial.println(oled_active);
    Serial.print("uss_active: ");Serial.println(uss_active);
    Serial.print("splash_active: ");Serial.println(splash_active);
    Serial.println();
  }

  //drive the self-reset pin HIGH *before* making it an output, or configuring
  //it would immediately reset the board
  digitalWrite(RESET_PIN, HIGH);
  delay(20);
  pinMode(RESET_PIN, OUTPUT);

  //join the i2c bus as a slave and register the two interrupt handlers
  Wire.begin(SLAVE_ID);
  Wire.onRequest(requestCallback);
  Wire.onReceive(receiveEvent);
  delay(200);

  pinMode(LED_BRIGHT_PIN, INPUT);
  pinMode(OLED_ACTIVE_PIN, INPUT);
  digitalWrite(OLED_ACTIVE_PIN, LOW);

  if (rgb_active) {
    led_eyes.begin();
    led_eyes.setBrightness(rgb_bright);
    wipe_eyes();
  }

  if (oled_active) {
    boot_splash();
  }

  if (debug1) Serial.println(F("Ready!"));

  //idle the eyes on the green wipe wave until the master says otherwise
  pattern = 12;
  pattern_cnt = 32;               //4 steps per sequence for pattern 12
  pattern_int = 50;
}


//the startup animation: colour bars, the Nova logo, then the flashing banner
void boot_splash() {
  display.begin();
  display.setFont();

  if (splash_active) {
    lcdTestPattern();
    delay(500);
  }

  display.fillScreen(BLACK);
  display.setTextSize(1);
  display.setTextColor(YELLOW);
  display.setCursor(0, 20);
  display.println("Initializing...");
  delay(2000);
  display.fillScreen(BLACK);

  if (splash_active) {
    //cycle the logo through three colours to make it shimmer
    for (int i = 0; i < 12; i++) {
      display.drawBitmap(-20, 10, smbmp, 128, 64, YELLOW);
      delay(60);
      display.drawBitmap(-20, 10, smbmp, 128, 64, WHITE);
      delay(30);
      display.drawBitmap(-20, 10, smbmp, 128, 64, MAGENTA);
      delay(60);
    }
  }

  display.fillScreen(BLACK);
  draw_name_banner(WHITE);
  delay(1500);

  if (splash_active) {
    //redraw the banner alternating colours, speeding up as it goes
    int use_yellow = 1;
    for (int i = 25; i > 0; i--) {
      int d = (i > 22) ? (i * 10) : (i * 2);
      draw_name_banner(use_yellow ? YELLOW : MAGENTA);
      use_yellow = !use_yellow;
      delay(d);
      display.fillScreen(BLACK);
      delay(d);
    }
  }

  display.fillScreen(BLACK);
  sleep_display(&display);
}


//"NOVA / SM3 v6.0" in the given colour, at the fixed splash layout
void draw_name_banner(unsigned int color) {
  display.setTextColor(color);
  display.setCursor(10, 10);
  display.setTextSize(3);
  display.println("NOVA");
  display.setCursor(10, 35);
  display.setTextSize(2);
  display.print("SM3");
  display.setCursor(50, 45);
  display.setTextSize(1);
  display.print("v");
  display.println(VERSION);
}


/*
   -------------------------------------------------------
   Loop
    :advance the eyes and the display, and watch the panel button

    Nothing here talks to the master - that all happens in the two i2c
    interrupt handlers below, which can fire at any point during this loop.
   -------------------------------------------------------
*/
void loop() {
  if (rgb_active) {
    if (millis() - lastRGBUpdate > rgbInterval) rgb_check(pattern);
  }
  if (oled_active) {
    if (millis() - lastOLEDUpdate > oledInterval) oled_check(oled_command);
    if (millis() - lastOLEDClear > oledClearInterval) oled_clear();
  }

  check_oled_button();
}


/*
   -------------------------------------------------------
   Check OLED Button
    :the front panel button toggles the display on and off

    Held for a full second to count, so a knock does not switch the screen
    off mid-run. The one-second confirmation is a blocking delay, which is
    acceptable here because the button is only ever pressed deliberately.
   -------------------------------------------------------
*/
void check_oled_button() {
  if (digitalRead(OLED_ACTIVE_PIN) != 1) return;

  if (debug1) Serial.println(F("btn active 1"));
  delay(1000);
  if (digitalRead(OLED_ACTIVE_PIN) != 1) return;   //released too soon

  if (debug1) Serial.println(F("btn active 2"));

  //flash the eyes so there is feedback even when the screen is going dark
  pattern_cnt = 3;
  pattern_int = 100;
  rgb_check(10);

  if (oled_active) {
    if (debug1) Serial.println(F("oled inactive"));
    display.begin();
    display.fillScreen(RED);
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(25, 15);
    display.print("OLED");
    display.setCursor(25, 35);
    display.print("OFF!");
    delay(3000);
    oled_active = 0;
    oled_clear();
  } else {
    if (debug1) Serial.println(F("oled active"));
    oled_active = 1;
//DEV_NOTE: need to check here if oled was active / initialized at boot
//          if not, activate to eprom and reboot?
    wake_display(&display);
    display.begin();
    display.fillScreen(GREEN);
    display.setTextColor(BLACK);
    display.setTextSize(2);
    display.setCursor(25, 15);
    display.print("OLED");
    display.setCursor(30, 35);
    display.print("ON!");
    delay(3000);
    oled_clear();
  }
}


/*
   -------------------------------------------------------
   Receive Event
    :i2c interrupt - the master has sent a command byte

    Only stores it. The master always follows a write with a request, so the
    work is done in requestCallback() where a response can be returned.
    If more than one byte arrives, the last one wins.
   -------------------------------------------------------
*/
void receiveEvent(int aCount) {
  while (0 < Wire.available()) {
    req = Wire.read();
  }
}

/*
   -------------------------------------------------------
   Request Callback
    :called by the Wire library when the master asks for a response

    By this point receiveEvent() has already stored the command byte in
    `req`. This decides what it meant, does it, and hands back two bytes.

    Which of the two command tables applies depends on serial_oled - see
    NovaSlaveProtocol.h, which is the definitive list of every byte and is
    shared with the master.
   -------------------------------------------------------
*/
void requestCallback() {
  byte send_resp = 0;
  resp = 0;

  if (debug1) {
    Serial.print(F("oled: "));Serial.print(oled_active);Serial.print("\t");
    Serial.print(F("rgb: "));Serial.print(rgb_bright);Serial.print("\t");
    Serial.print(serial_oled ? F("OLED Command ") : F("System Command "));
    Serial.print(F("req: "));
    Serial.println(req);
  }

  if (serial_oled) {
    send_resp = handle_oled_command(req);
  } else {
    send_resp = handle_system_command(req);
  }

  if (send_resp) {
    if (debug1) Serial.print(F("Sending response..."));
    uint8_t buffer[2];
    buffer[0] = resp >> 8;
    buffer[1] = resp & 0xff;
    Wire.write(buffer, 2);
    if (debug1) Serial.println(F("sent!"));
  }
}


/*
   -------------------------------------------------------
   OLED-mode commands
    :sets which screen loop() should draw next. Returns 1 if the master is
    :waiting for a response byte.

    Anything not handled here is taken to be a screen id and stored in
    oled_command for oled_check() to draw - that is why there is no error
    case: the screen list lives in oled_check(), not here.
   -------------------------------------------------------
*/
byte handle_oled_command(char cmd) {
  //battery level to use the next time the gauge is drawn
  if (cmd >= CMD_BYTE(OLED_BATTERY_LEVEL_0) && cmd <= CMD_BYTE(OLED_BATTERY_LEVEL_9)) {
    blevel_id = cmd - CMD_BYTE(OLED_BATTERY_LEVEL_0);
    return 0;
  }

  if (cmd == CMD_BYTE(SLAVE_TOGGLE_MODE)) {
    //back to system commands, and tell the master which mode we are in now
    serial_oled = 0;
    resp = serial_oled;
    return 1;
  }

  if (cmd == CMD_BYTE(SLAVE_REBOOT_SPLASH)) {
    reset_slave();
    return 0;
  }

  //everything else names a screen
  oled_command = cmd;
  return cmd;
}


/*
   -------------------------------------------------------
   System-mode commands
    :ultrasonic reads, the RGB eye animation, and the mode / reboot bytes.
    :Returns 1 if the master is waiting for a response byte.

    The RGB commands come in four contiguous blocks of bytes - pattern,
    repeat count, step interval and colour - so each block is a range test
    and a small lookup table rather than a case per byte. The ranges are
    written in terms of the protocol constants, so they follow automatically
    if a block is ever extended.
   -------------------------------------------------------
*/
byte handle_system_command(char cmd) {
  //--- ultrasonic sensors: the only commands that return a reading ---
  if (cmd == CMD_BYTE(USS_READ_LEFT)) {
    resp = get_distance(SONAR_LEFT);
    return 1;
  }
  if (cmd == CMD_BYTE(USS_READ_RIGHT)) {
    resp = get_distance(SONAR_RIGHT);
    return 1;
  }

  //--- which animation to run ---
  //RGB_FADE_BLUE ('d') through RGB_PATTERN_OFF ('p'), in protocol order
  if (cmd >= CMD_BYTE(RGB_FADE_BLUE) && cmd <= CMD_BYTE(RGB_PATTERN_OFF)) {
    static const byte pattern_for[] PROGMEM = {
      1,  //RGB_FADE_BLUE
      2,  //RGB_FADE_ORANGE
      3,  //RGB_FADE_GREEN
      4,  //RGB_FADE_YELLOW
      5,  //RGB_FADE_RED
      6,  //RGB_FADE_PURPLE
      7,  //RGB_SOLID_WHITE
      8,  //RGB_WIPE_PAIRED
      9,  //RGB_FLOW
      10, //RGB_RAINBOW
      11, //RGB_PATTERN_BLINK
      12, //RGB_WIPE_WAVE
      0,  //RGB_PATTERN_OFF
    };
    pattern = pgm_read_byte(&pattern_for[cmd - CMD_BYTE(RGB_FADE_BLUE)]);
    return 0;
  }

  //--- how many times it repeats ---
  //RGB_COUNT_0 ('s') through RGB_COUNT_25 ('z'), then RGB_COUNT_100 ('A')
  if (cmd >= CMD_BYTE(RGB_COUNT_0) && cmd <= CMD_BYTE(RGB_COUNT_25)) {
    static const byte count_for[] PROGMEM = { 0, 1, 2, 3, 4, 5, 10, 25 };
    pattern_cnt = pgm_read_byte(&count_for[cmd - CMD_BYTE(RGB_COUNT_0)]);
    return 0;
  }
  if (cmd == CMD_BYTE(RGB_COUNT_100)) {
    pattern_cnt = 100;
    return 0;
  }

  //--- how fast it steps ---
  //RGB_SPEED_30 ('D') through RGB_SPEED_5000 ('K')
  if (cmd >= CMD_BYTE(RGB_SPEED_30) && cmd <= CMD_BYTE(RGB_SPEED_5000)) {
    static const unsigned int speed_for[] PROGMEM = { 30, 50, 100, 250, 500, 1000, 2500, 5000 };
    pattern_int = fade_steps = pgm_read_word(&speed_for[cmd - CMD_BYTE(RGB_SPEED_30)]);
    return 0;
  }

  //--- which eye the next colour applies to ---
  if (cmd == CMD_BYTE(RGB_LEFT))  { pattern_side = 0; return 0; }
  if (cmd == CMD_BYTE(RGB_RIGHT)) { pattern_side = 1; return 0; }

  //--- the colour itself ---
  //RGB_RED ('O') through RGB_GREY ('V')
  if (cmd >= CMD_BYTE(RGB_RED) && cmd <= CMD_BYTE(RGB_GREY)) {
    static const byte eye_colors[][3] PROGMEM = {
      { 255,   0,   0 },  //RGB_RED
      {   0, 255,   0 },  //RGB_GREEN
      {   0,   0, 255 },  //RGB_BLUE
      { 255, 255,  55 },  //RGB_YELLOW
      {  55,   0, 200 },  //RGB_PURPLE
      { 255, 155,   0 },  //RGB_ORANGE
      { 255, 255, 255 },  //RGB_WHITE
      { 155, 155, 155 },  //RGB_GREY
    };
    byte i = cmd - CMD_BYTE(RGB_RED);
    int* eye = pattern_side ? cur_rgb_val2 : cur_rgb_val1;
    for (byte c = 0; c < 3; c++) {
      eye[c] = pgm_read_byte(&eye_colors[i][c]);
    }
    return 0;
  }

  //--- mode and reboot ---
  if (cmd == CMD_BYTE(SLAVE_TOGGLE_MODE)) {
    //switch to display commands, and tell the master which mode we are in now
    serial_oled = 1;
    resp = serial_oled;
    return 1;
  }
  if (cmd == CMD_BYTE(SLAVE_REBOOT_QUIET) || cmd == CMD_BYTE(SLAVE_REBOOT_SPLASH)) {
    //the master decides whether the splash plays; remember it across the
    //reboot so the setting survives a power cycle too
    splash_active = (cmd == CMD_BYTE(SLAVE_REBOOT_SPLASH));
    EEPROM.write(EEPROM_SPLASH_ACTIVE, splash_active);
    delay(300);
    reset_slave();
    return 0;
  }

  //unrecognised, or one of the bytes reserved for future use
  return 0;
}


/*
   -------------------------------------------------------
   Get Distance
    :take one ultrasonic reading, in centimetres

    `side` is SONAR_LEFT or SONAR_RIGHT. A reading further away than
    MAX_DISTANCE is returned to the master but not remembered, so the cached
    dist_l / dist_r the display shows always hold a plausible value.

    NOTE: the return is the raw reading, which is 0 when nothing echoes back.
    The master treats 0 as "no obstacle", not "touching something".
   -------------------------------------------------------
*/
int get_distance(int side) {
  int dist = sonar[side].ping_cm();

  if (dist <= MAX_DISTANCE) {
    if (side == SONAR_RIGHT) {
      dist_r = dist;
    } else {
      dist_l = dist;
    }
  }

  //BUGFIX (v6.0): only the first of these three prints was guarded by debug1,
  //so the last two ran on every reading and spammed the serial port even with
  //debugging off. All three are now inside the guard.
  if (debug1) {
    Serial.print((side == SONAR_RIGHT) ? F("Right ultrasonic sensor:\t")
                                       : F("Left ultrasonic sensor:\t\t"));
    Serial.print(side); Serial.print(" : "); Serial.println(dist);
  }

  return dist;
}
