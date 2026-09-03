/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Servo state and motion setup
 *
 *   The layer between the movement routines and AsyncServo. Nothing here
 *   moves a servo directly - it sets up the position, speed, ramp and sweep
 *   parameters that AsyncServo::Update() acts on during later passes of
 *   loop(). init_home() is the exception: it drives the servos at boot.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   Operational Functions
   -------------------------------------------------------
*/
void init_home() {
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    for (int j = 0; j < 6; j++) {
      servoSweep[i][j] = 0;
    }
    for (int j = 0; j < 8; j++) {
      servoRamp[i][j] = 0;
    }

    servoDelay[i][0] = 0;
    servoDelay[i][1] = 0;
    servoStep[i] = 0;
    servoSwitch[i] = 0;
    servoSpeed[i] = (spd * 1.5);
    activeSweep[i] = 0;
  }

  for (int i = 0; i < TOTAL_LEGS; i++) {
    servoSequence[i] = 0;
  }

  //set crouched positions
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    if (is_tibia(i)) {
      if (is_left_leg(i)) {
        servoPos[i] = (servoLimit[i][1] + 40);
      } else {
        servoPos[i] = (servoLimit[i][1] - 40);
      }
    } else if (is_femur(i)) {
      if (is_left_leg(i)) {
        servoPos[i] = (servoLimit[i][0] - 40);
      } else {
        servoPos[i] = (servoLimit[i][0] + 40);
      }
    } else {
      servoPos[i] = servoHome[i];
    }
  }

  //intitate servos in groups
  //coaxes
  pwm1.setPWM(servoSetup[RFC][1], 0, servoPos[RFC]);
  pwm1.setPWM(servoSetup[LRC][1], 0, servoPos[LRC]);
  pwm1.setPWM(servoSetup[RRC][1], 0, servoPos[RRC]);
  pwm1.setPWM(servoSetup[LFC][1], 0, servoPos[LFC]);
  delay(1000);

  //tibias
  pwm1.setPWM(servoSetup[RFT][1], 0, servoPos[RFT]);
  pwm1.setPWM(servoSetup[LRT][1], 0, servoPos[LRT]);
  pwm1.setPWM(servoSetup[RRT][1], 0, servoPos[RRT]);
  pwm1.setPWM(servoSetup[LFT][1], 0, servoPos[LFT]);
  delay(1000);

  //femurs
  pwm1.setPWM(servoSetup[RFF][1], 0, servoPos[RFF]);
  pwm1.setPWM(servoSetup[LRF][1], 0, servoPos[LRF]);
  pwm1.setPWM(servoSetup[RRF][1], 0, servoPos[RRF]);
  pwm1.setPWM(servoSetup[LFF][1], 0, servoPos[LFF]);
  delay(1000);

  set_stay();
}


void detach_all() {
  if (debug4) {
    Serial.println(F("detaching all servos!"));
  }
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    activeServo[i] = 0;
    activeSweep[i] = 0;
    pwm1.setPWM(i, 0, 0);
  }
  digitalWrite(OE_PIN, HIGH);
  digitalWrite(PWR_PIN, LOW);
  pwm_active = 0;
  amp_active = 0;

  if (rgb_active) {
    rgb_request(RGB_LEFT RGB_RED RGB_RIGHT RGB_YELLOW RGB_COUNT_100 RGB_SPEED_250 RGB_PATTERN_BLINK);
  }
}


/*
   -------------------------------------------------------
   Set Sweep
    :arm one servo to sweep back and forth between two positions

    servo  : servo ID (RFC, LFF, ... - see NovaServos.h)
    speed  : ms delay between pulses; SMALLER IS FASTER (see min_spd / max_spd)
    from   : one end of the travel, in raw pwm ticks
    to     : the other end, and the position the servo heads for first
    loops  : how many out-and-back passes before the sweep stops

    Both `from` and `to` are expected to be pre-clamped by the caller, usually
    with limit_target(), because several callers deliberately clamp with a
    safety threshold rather than to the bare travel limit.

    AsyncServo::Update() does the actual moving; this only sets the parameters
    it reads. The per-sweep delay slot servoSweep[servo][4] is intentionally
    left alone here, since no caller uses it and init_home() already zeroes it.
   -------------------------------------------------------
*/
void set_sweep(int servo, float speed, float from, float to, int loops) {
  servoSpeed[servo] = speed;
  servoSweep[servo][SWEEP_FROM]  = from;
  servoSweep[servo][SWEEP_TO]    = to;
  servoSweep[servo][SWEEP_DIR]   = 0;      //start out heading for `to`
  servoSweep[servo][SWEEP_LOOPS] = loops;
  targetPos[servo] = to;
  activeSweep[servo] = 1;
}


/*
   -------------------------------------------------------
   Set Ramp
    :build an acceleration / deceleration profile for one servo

    servo    : servo ID
    sp       : the cruising speed the servo should settle at
    r1_spd   : speed to start from, 0 = derive it from ramp_spd
    r1_dist  : how far the acceleration lasts, 0 = derive it from ramp_dist
    r2_spd   : speed to finish at,   0 = derive it from ramp_spd
    r2_dist  : how far the deceleration lasts, 0 = derive it from ramp_dist

    Remember speeds are inter-tick DELAYS, so the ramp-up speed is a bigger
    number than the cruising speed - the servo starts slow and gets quicker.

    Only sets up the profile; AsyncServo::interpolateRamp() walks through it
    one tick at a time. Nothing happens unless the global use_ramp is set.

    KNOWN ISSUE carried over from 5.1: interrupting a move part way through a
    ramp leaves servoSpeed at whatever the ramp had reached, rather than
    restoring the cruising speed.
   -------------------------------------------------------
*/
void set_ramp(int servo, float sp, float r1_spd, float r1_dist, float r2_spd, float r2_dist) {
  servoRamp[servo][RAMP_SPEED] = sp;
  servoRamp[servo][RAMP_DISTANCE] = abs(servoPos[servo] - targetPos[servo]);

  //fall back to the global ramp shape for anything the caller left at 0
  if (!r1_spd) r1_spd = sp + (sp * ramp_spd);
  if (!r2_spd) r2_spd = sp + (sp * ramp_spd);
  if (!r1_dist) r1_dist = (servoRamp[servo][RAMP_DISTANCE] * ramp_dist);
  if (!r2_dist) r2_dist = (servoRamp[servo][RAMP_DISTANCE] * ramp_dist);

  //acceleration phase
  servoRamp[servo][RAMP_UP_SPEED] = r1_spd;
  servoRamp[servo][RAMP_UP_DIST] = r1_dist;
  servoRamp[servo][RAMP_UP_INC] = (servoRamp[servo][RAMP_UP_SPEED] - sp);
  if (r1_dist != 0) {
    //spread the whole speed change evenly across the ramp distance
    servoRamp[servo][RAMP_UP_INC] = (servoRamp[servo][RAMP_UP_SPEED] - sp) / r1_dist;
  }

  //deceleration phase
  servoRamp[servo][RAMP_DOWN_SPEED] = r2_spd;
  servoRamp[servo][RAMP_DOWN_DIST] = r2_dist;
  servoRamp[servo][RAMP_DOWN_INC] = (servoRamp[servo][RAMP_DOWN_SPEED] - sp);
  if (r2_dist != 0) {
    servoRamp[servo][RAMP_DOWN_INC] = (servoRamp[servo][RAMP_DOWN_SPEED] - sp) / r2_dist;
  }

  if (debug3 && servo == debug_servo) {
    Serial.print("set_ramp: sPos: "); Serial.print(servoPos[servo]);
    Serial.print("\ttPos: "); Serial.print(targetPos[servo]);
    Serial.print("\tr1_dist: "); Serial.print(r1_dist);
    Serial.print("\tr2_dist: "); Serial.println(r2_dist);
    Serial.print("ramp:");
    for (int i = 0; i < 8; i++) {
      Serial.print("\t"); Serial.print(servoRamp[servo][i]);
    }
    Serial.println();
    Serial.println();
  }
}


void go_home() {
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    activeServo[i] = 0;
    activeSweep[i] = 0;
    servoSpeed[i] = spd;
    servoPos[i] = servoHome[i];
    targetPos[i] = servoHome[i];
    if (servoSetup[i][0] == 1) {
      pwm1.setPWM(servoSetup[i][1], 0, servoHome[i]);
    }
    delay(20);
  }

  for (int i = 0; i < TOTAL_LEGS; i++) {
    servoSequence[i] = 0;
  }
}


void set_home() {
  for (int m = 0; m < TOTAL_SERVOS; m++) {
    activeServo[m] = 1;
    targetPos[m] = servoHome[m];
  }
}


void set_stop() {
  for (int m = 0; m < TOTAL_SERVOS; m++) {
    activeServo[m] = 0;
    activeSweep[m] = 0;
  }
  for (int l = 0; l < TOTAL_LEGS; l++) {
    servoSequence[l] = 0;
  }
  set_home();
}


void set_stop_active() {
  for (int m = 0; m < TOTAL_SERVOS; m++) {
    activeServo[m] = 0;
    activeSweep[m] = 0;
  }
  for (int l = 0; l < TOTAL_LEGS; l++) {
    servoSequence[l] = 0;
  }
  use_ramp = 0;

  moving = 0;
  move_y_axis = 0;
  move_x_axis = 0;
  move_roll = 0;
  move_roll_body = 0;
  move_pitch = 0;
  move_pitch_body = 0;
  move_trot = 0;
  move_forward = 0;
  move_backward = 0;
  move_left = 0;
  move_right = 0;
  move_march = 0;
  move_wake = 0;
  move_sequence = 0;
  move_demo = 0;
  move_wman = 0;
  move_funplay = 0;
  move_look_left = 0;
  move_look_right = 0;
  move_roll_x = 0;
  move_pitch_y = 0;
  move_kin_x = 0;
  move_kin_y = 0;
  move_yaw_x = 0;
  move_yaw_y = 0;
  move_yaw = 0;
  move_servo = 0;
  move_leg = 0;
  move_follow = 0;

  if (mpu_is_active) mpu_active = 1;
}


void set_speed() {
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    servoSpeed[i] = spd;
  }
  //recalc speed factor
  spd_factor = mapfloat(spd, min_spd, max_spd, min_spd_factor, max_spd_factor);
  if (rgb_active) {
    if (spd < 6) {
      rgb_request(RGB_LEFT RGB_RED RGB_RIGHT RGB_RED RGB_SPEED_30);
    } else if (spd < 11) {
      rgb_request(RGB_LEFT RGB_YELLOW RGB_RIGHT RGB_YELLOW RGB_SPEED_50);
    } else if (spd < 21) {
      rgb_request(RGB_LEFT RGB_GREEN RGB_RIGHT RGB_GREEN RGB_SPEED_100);
    } else if (spd < 31) {
      rgb_request(RGB_LEFT RGB_BLUE RGB_RIGHT RGB_BLUE RGB_SPEED_250);
    } else {
      rgb_request(RGB_LEFT RGB_PURPLE RGB_RIGHT RGB_PURPLE RGB_SPEED_500);
    }
    rgb_request(RGB_COUNT_5 RGB_PATTERN_BLINK);
  }
}


void move_debug_servo() {
  if (!activeSweep[debug_servo]) {
    set_sweep(debug_servo, spd, servoLimit[debug_servo][0], servoLimit[debug_servo][1], 1);

    if (debug_loops2) {
      debug_loops2--;
    }
    if (!debug_loops2) {
      set_stop_active();
      set_home();
    } 
  }
}


void move_debug_leg() {
  //check if leg active
  int lactive = 0;
  for (int i = 0; i < 3; i++) {
    int dservo = servoLeg[debug_leg][i];
    if (activeSweep[dservo]) {
      lactive = 1;
    }
  }

  if (!lactive) {
    for (int i = 0; i < 3; i++) {
      int dservo = servoLeg[debug_leg][i];
      if (!activeSweep[dservo]) {
        //sweep from one travel limit to the other. which limit comes first is
        //flipped for front legs, and flipped again for femurs, so that every
        //joint visibly moves outwards from home first rather than into itself
        byte reversed = (is_front_leg(dservo) != is_femur(dservo));
        set_sweep(dservo, debug_spd,
                  servoLimit[dservo][reversed ? 1 : 0],
                  servoLimit[dservo][reversed ? 0 : 1], 1);
      }
    }

    if (debug_loops2) {
      debug_loops2--;
    } 
    if (!debug_loops2) {
      set_stop_active();
      set_home();
    } 
  }
}
