/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Small shared helpers
 *
 *   Clamping, unit conversion, and the "which kind of joint is this?"
 *   predicates the movement routines lean on constantly.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   General Functions
   -------------------------------------------------------
*/
int limit_target(int sid, int tar, int thresh) {
  if (servoLimit[sid][0] > servoLimit[sid][1]) {
    if ((tar + thresh) > servoLimit[sid][0]) {
      tar = (servoLimit[sid][0] - thresh);
    } else if ((tar - thresh) < servoLimit[sid][1]) {
      tar = (servoLimit[sid][1] + thresh);
    }
  } else {
    if ((tar - thresh) < servoLimit[sid][0]) {
      tar = (servoLimit[sid][0] + thresh);
    } else if ((tar + thresh) > servoLimit[sid][1]) {
      tar = (servoLimit[sid][1] - thresh);
    }
  }

  return tar;
}


int limit_speed(float spd_lim) {
  if (spd_lim > min_spd) spd_lim = min_spd;
  if (spd_lim < max_spd) spd_lim = max_spd;

  return spd_lim;
}


byte is_stepmove_complete(int ms) {
  byte ret = 1;
  for (int m = 0; m < TOTAL_SERVOS; m++) {
    if (servoPos[m] == servoStepMoves[m][ms-1]) ret = 0;  
  }

  return ret;
}


byte is_front_leg(int leg) {
  if (leg == LFC || leg == LFF || leg == LFT || leg == RFC || leg == RFF || leg == RFT) 
    return 1;
  else 
    return 0;
}


byte is_left_leg(int leg) {
  if (leg == LFC || leg == LFF || leg == LFT || leg == LRC || leg == LRF || leg == LRT)
    return 1;
  else
    return 0;
}


byte is_femur(int leg) {
  if (leg == RFF || leg == LFF || leg == RRF || leg == LRF)
    return 1;
  else
    return 0;
}


byte is_tibia(int leg) {
  if (leg == RFT || leg == LFT || leg == RRT || leg == LRT)
    return 1;
  else
    return 0;
}


float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


int degrees_to_pwm(int pangle, int mxw, int mnw, int rng) {
  int pulse_wide = map(pangle, -rng/2, rng/2, mnw, mxw);

  return pulse_wide;
}


int pwm_to_degrees(int pulse_wide, int mxw, int mnw, int rng) {
  int pangle = map(pulse_wide, mnw, mxw, -rng/2, rng/2);

  return pangle;
}
