/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Arduino Nano (slave)
 *
 *   OLED display
 *
 *   Everything drawn on the 96x64 SSD1331. The master never sends pixels -
 *   it sends a one-byte screen id, oled_check() draws the matching screen,
 *   and oled_clear() blanks the panel again once nothing new has arrived.
 *
 *   One tab of the Nova-SM3_nano-v6.0 sketch.
*/


/*
   -------------------------------------------------------
   OLED Check
    :draw whichever screen the master last asked for

    Called from loop() on the oledInterval timer. oled_command holds the
    pending screen id, or 0 when there is nothing to draw; it is cleared here
    so each screen is drawn once.

    The ids are the OLED_* constants in NovaSlaveProtocol.h. CMD_BYTE()
    turns each one-character string into the character this compares against,
    so the master and this board read their meanings from the same file.
   -------------------------------------------------------
*/
void oled_check(char cmd) {
  if (cmd > 0) {
    if (debug1) { Serial.print(F("Print to OLED: ")); Serial.println(cmd); }

    wake_display(&display);
    display.fillScreen(BLACK);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    if (cmd == CMD_BYTE(OLED_WAKE)) {
      display.println("Grrrrr!");

    } else if (cmd == CMD_BYTE(OLED_STAY)) {
      display.println("Stay!");

    } else if (cmd == CMD_BYTE(OLED_DISTANCES)) {
      //live readout of the two ultrasonic sensors
      display.setTextSize(3);
      display.setCursor(0, 10);
      display.setTextColor(MAGENTA);
      display.print("L:");
      display.setTextColor(YELLOW);
      display.println(dist_l);
      display.setCursor(0, 40);
      display.setTextColor(MAGENTA);
      display.print("R:");
      display.setTextColor(YELLOW);
      display.println(dist_r);

    } else if (cmd == CMD_BYTE(OLED_MARCH)) {
      display.println("March!");

    } else if (cmd == CMD_BYTE(OLED_INTRUDER)) {
      display.println("INTRUDER");
      display.setTextSize(3);
      display.setCursor(0, 15);
      display.println("ALERT!");

    } else if (cmd == CMD_BYTE(OLED_ALERT_CLEAR)) {
      display.println("ALERT");
      display.setCursor(0, 19);
      display.println("COMPLETE!");

    } else if (cmd == CMD_BYTE(OLED_MODE_1)) {
      display.println("Mode 1");
    } else if (cmd == CMD_BYTE(OLED_MODE_2)) {
      display.println("Mode 2");
    } else if (cmd == CMD_BYTE(OLED_MODE_3)) {
      display.println("Mode 3");
    } else if (cmd == CMD_BYTE(OLED_MODE_4)) {
      display.println("Mode 4");

    } else if (cmd == CMD_BYTE(OLED_HALTED)) {
      display.setCursor(10, 10);
      display.setTextColor(MAGENTA);
      display.println("SYSTEM");
      display.setTextSize(2);
      display.setCursor(8, 30);
      display.setTextColor(RED);
      display.println("HALTED!");

    } else if (cmd == CMD_BYTE(OLED_READY)) {
      display.println("Ready!");

    } else if (cmd == CMD_BYTE(OLED_ALARM)) {
      alarm_display();

    } else if (cmd == CMD_BYTE(OLED_BATTERY_GAUGE)) {
      battery_gauge();

    } else if (cmd == CMD_BYTE(OLED_STAND_BACK)) {
      display.fillScreen(BLACK);
      display.setTextSize(2);
      display.setTextColor(YELLOW);
      display.setCursor(10, 4);
      display.println("PLEASE");
      display.setTextColor(RED);
      display.setCursor(15, 24);
      display.println("STAND");
      display.setCursor(12, 44);
      display.println("BACK!!");

    //radar sweep: the circle centre shows heading, the count shows how far
    } else if (cmd == CMD_BYTE(OLED_RADAR_FORWARD)) {
      radar_display(48, 36);
    } else if (cmd == CMD_BYTE(OLED_RADAR_FWD_LEFT)) {
      radar_display(60, 24);
    } else if (cmd == CMD_BYTE(OLED_RADAR_FWD_RIGHT)) {
      radar_display(36, 24);
    } else if (cmd == CMD_BYTE(OLED_RADAR_LEFT)) {
      radar_display(72, 18);
    } else if (cmd == CMD_BYTE(OLED_RADAR_RIGHT)) {
      radar_display(24, 18);
    } else if (cmd == CMD_BYTE(OLED_RADAR_BACKWARD)) {
      radar_display(48, 12);
    } else if (cmd == CMD_BYTE(OLED_RADAR_GREET)) {
      radar_display(48, 12);
    } else if (cmd == CMD_BYTE(OLED_RADAR_STOP)) {
      radar_display(48, 6);
    }

    oled_command = 0;
    oledClearInterval = 5000;
    lastOLEDClear = millis();
  }

  lastOLEDUpdate = millis();
}


//reserved for a future alarm graphic; currently just blanks the screen
void alarm_display() {
  display.fillScreen(BLACK);
//DEV: future use
}


/*
   -------------------------------------------------------
   Radar Display
    :a radar-style sweep showing which way Nova is heading

    cir_x    : x centre of the rings, which is what indicates direction
    cir_cnt  : radius the rings grow to, which indicates distance

    The grid is drawn by painting a full yellow cross and then blacking out
    3-pixel bands, which is cheaper on this display than drawing each
    surviving segment. The band positions are the arrays below.
   -------------------------------------------------------
*/
void radar_display(int cir_x, int cir_cnt) {
  //3px-wide gaps knocked out of the grid. 48 / 32 are left intact - they are
  //the axes drawn just below.
  static const byte grid_gap_x[] PROGMEM = { 12, 24, 36, 60, 72, 84 };
  static const byte grid_gap_y[] PROGMEM = { 12, 22, 40, 50 };
  const byte GAP_WIDTH = 3;

  display.fillScreen(BLACK);

  //the axes
  display.drawLine(0, 32, SCREEN_WIDTH, 32, YELLOW);
  display.drawLine(48, 0, 48, SCREEN_HEIGHT, YELLOW);
  delay(100);

  for (byte i = 0; i < sizeof(grid_gap_x); i++) {
    for (byte w = 0; w < GAP_WIDTH; w++) {
      byte x = pgm_read_byte(&grid_gap_x[i]) + w;
      display.drawLine(x, 0, x, SCREEN_HEIGHT, BLACK);
    }
  }
  for (byte i = 0; i < sizeof(grid_gap_y); i++) {
    for (byte w = 0; w < GAP_WIDTH; w++) {
      byte y = pgm_read_byte(&grid_gap_y[i]) + w;
      display.drawLine(0, y, SCREEN_WIDTH, y, BLACK);
    }
  }

  //rings out in white, then back in green, which reads as a sweep
  if (cir_x) {
    for (int i = 5; i < cir_cnt; i += 5) {
      delay(50);
      display.drawCircle(cir_x, 32, i, WHITE);
    }
    for (int i = cir_cnt; i > 5; i -= 5) {
      delay(50);
      display.drawCircle(cir_x, 32, i, GREEN);
    }
  }
}


/*
   -------------------------------------------------------
   Battery Gauge
    :five bars that shrink and change colour as the pack drains

    The master works out which of the ten levels the pack has reached and
    sends it ahead of the gauge command; blevel_id holds it, 0 = healthiest.

    The bars are a straight table lookup on that level. The original wrote
    out all ten rows as an if/else cascade comparing voltages, but since the
    voltage compared against is simply batt_levels[blevel_id], every one of
    those tests resolved to "which level is it" - so the level indexes the
    table directly.
   -------------------------------------------------------
*/
void battery_gauge() {
  //bar heights in pixels, one row per level, tallest bar on the right
  static const byte bar_height[10][5] PROGMEM = {
    { 15, 20, 25, 30, 35 },   //0  healthiest
    { 15, 20, 25, 30, 30 },   //1
    { 15, 20, 25, 25, 30 },   //2
    { 15, 20, 20, 25, 30 },   //3
    { 10, 15, 20, 25, 30 },   //4
    {  5, 10, 15, 20, 25 },   //5
    {  5, 10, 15, 20, 20 },   //6
    {  5,  5, 10, 15, 20 },   //7
    {  5,  5,  5, 10, 15 },   //8
    {  5,  5,  5,  5,  5 },   //9  critical
  };
  //0 = green, 1 = yellow, 2 = red
  static const byte bar_shade[10][5] PROGMEM = {
    { 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1 },
    { 0, 0, 0, 1, 1 },
    { 0, 0, 1, 1, 1 },
    { 0, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 2 },
    { 1, 1, 1, 2, 2 },
    { 1, 1, 2, 2, 2 },
    { 1, 2, 2, 2, 2 },
    { 2, 2, 2, 2, 2 },
  };
  static const unsigned int shade_color[3] PROGMEM = { GREEN, YELLOW, RED };
  static const byte bar_x[5] PROGMEM = { 2, 20, 38, 56, 74 };
  const byte BAR_WIDTH = 16;
  const byte BAR_BASE = 60;     //all bars sit on this line and grow upwards

  byte level = blevel_id;
  if (level > 9) level = 9;
  float blevel = batt_levels[level];

  display.fillScreen(BLACK);
  display.setCursor(0, 0);

  if (level >= 8) {
    //levels 8 and 9: stop reporting a number and start shouting
    display.setTextSize(2);
    display.setTextColor(YELLOW);
    const char* danger = "DANGER";
    for (byte i = 0; danger[i]; i++) {
      display.print(danger[i]);
      delay(50);
    }
    display.println("!!");
    delay(200);

    display.setTextColor(CYAN);
    display.setCursor(0, 22);
    display.println("CHARGE");
    display.setCursor(0, 42);
    display.println(" BATTERY");
  } else {
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.print("Bat ");
    display.setTextSize(2);
    if (level == 0) {
      //still above the top of the ladder, so do not imply a precise reading
      display.setTextColor(GREEN);
      display.print("+11 ");
    } else if (level <= 3) {
      display.setTextColor(GREEN);
      display.print(blevel);
    } else {
      display.setTextColor(YELLOW);
      display.print(blevel);
    }
    display.setTextColor(MAGENTA);
    display.println("v");
  }
  delay(10);

  for (byte i = 0; i < 5; i++) {
    byte h = pgm_read_byte(&bar_height[level][i]);
    byte shade = pgm_read_byte(&bar_shade[level][i]);
    display.fillRect(pgm_read_byte(&bar_x[i]), BAR_BASE - h, BAR_WIDTH, h,
                     pgm_read_word(&shade_color[shade]));
  }
}


/*
   -------------------------------------------------------
   OLED Clear
    :blank and sleep the panel once nothing new has been drawn for a while

    oledClearInterval is set to a very large value afterwards so this only
    fires once. oled_check() resets it to 5 seconds each time it draws.
   -------------------------------------------------------
*/
void oled_clear() {
  if (oled_command == 0) {
    if (debug1) Serial.println(F("Clear OLED"));
    display.fillScreen(BLACK);
    sleep_display(&display);
  }
  oledClearInterval = 5000000;
  lastOLEDClear = millis();
}


//colour bar test pattern, drawn once at boot when splash_active is set
void lcdTestPattern(void) {
  //eight vertical bands across the panel, widest colour last
  static const unsigned int bands[8] PROGMEM = {
    BLACK, YELLOW, MAGENTA, RED, CYAN, GREEN, BLUE, WHITE
  };
  display.setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  for (uint8_t h = 0; h < SCREEN_HEIGHT; h++) {
    for (uint8_t w = 0; w < SCREEN_WIDTH; w++) {
      byte band = w / 12;
      if (band > 7) band = 7;
      display.writePixel(w, h, pgm_read_word(&bands[band]));
    }
  }
  display.endWrite();
}


void sleep_display(Adafruit_SSD1331* display) {
    display->enableDisplay(0);
}

void wake_display(Adafruit_SSD1331* display) {
    display->enableDisplay(1);
}


