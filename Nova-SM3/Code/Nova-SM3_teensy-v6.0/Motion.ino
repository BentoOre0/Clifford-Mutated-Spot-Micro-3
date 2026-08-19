/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Body attitude
 *
 *   Moving the body without moving the feet: roll, pitch, yaw and the
 *   axis wiggles, plus set_axis() which is how IMU readings become servo
 *   positions. The multipliers throughout are hand-tuned to this frame.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   Move Functions
   -------------------------------------------------------
*/
//set pitch and roll axis from mpu data
void set_axis(float roll_step, float pitch_step) {
    float ar = abs(roll_step);
    float ap = abs(pitch_step);

    for (int i = 0; i < TOTAL_SERVOS; i++) {
      byte skip = 0;
      float t = 0.0;
      float f = 0.0;
      if (is_tibia(i)) {
        t = servoHome[i];
      } else if (is_femur(i)) {
        f = servoHome[i];
      }

      if (ar <= (mroll_prev + mpu_trigger_thresh) && ar >= (mroll_prev - mpu_trigger_thresh)) {
        if (roll_step < 0) { //roll left
          if (is_tibia(i)) {
            t -= ((abs(roll_step) * 0.65) * 4);
          } else if (is_femur(i)) {
            f += ((abs(roll_step) * 0.4) * 4);
          }
        } else { //roll right
          if (is_tibia(i)) {
            t += ((roll_step * 0.65) * 4);
          } else if (is_femur(i)) {
            f -= ((roll_step * 0.4) * 4);
          }
        }
      } else {
        skip = 1;
      }
      mroll_prev = ar;

      if (ap <= (mpitch_prev + mpu_trigger_thresh) || ap >= (mpitch_prev - mpu_trigger_thresh)) {
        if (pitch_step < 0) { //pitch front down
          if (is_tibia(i)) {
            if (is_front_leg(i)) {
              (is_left_leg(i)) ? t -= ((abs(pitch_step) * 1.15) * 3) : t += ((abs(pitch_step) * 1.15) * 3);
            } else {
              (is_left_leg(i)) ? t += ((abs(pitch_step) * 1.15) * 3) : t -= ((abs(pitch_step) * 1.15) * 3);
            }
          }
        } else { //pitch front up
          if (is_tibia(i)) {
            if (is_front_leg(i)) {
              (is_left_leg(i)) ? t += ((abs(pitch_step) * 1.15) * 3) : t -= ((abs(pitch_step) * 1.15) * 3);
            } else {
              (is_left_leg(i)) ? t -= ((abs(pitch_step) * 1.15) * 3) : t += ((abs(pitch_step) * 1.15) * 3);
            }
          }
        }
        mpitch_prev = pitch_step;
      }
      
      if (!skip) {
        if (is_tibia(i)) {
          activeServo[i] = 1;
          servoSpeed[i] = (7*spd_factor);
          targetPos[i] = limit_target(i, t, 0);
        } else if (is_femur(i)) {
          activeServo[i] = 1;
          servoSpeed[i] = (12*spd_factor);
          targetPos[i] = limit_target(i, f, 0);
        }
      }
    }

}


/*
   -------------------------------------------------------
   (implied) Kinematics Functions
   -------------------------------------------------------
*/

void move_kx() {
  int fms = (move_steps_kx * 0.8);
  int tms = (move_steps_kx * 1.3);
  int fsp = limit_speed((24 * spd_factor));
  int tsp = limit_speed((14 * spd_factor));

  update_sequencer(RF, RFF, fsp, (servoHome[RFF] - fms), 0, 0);
  update_sequencer(RF, RFT, tsp, (servoHome[RFT] + tms), 1, 0);
  update_sequencer(LF, LFF, fsp, (servoHome[LFF] + fms), 0, 0);
  update_sequencer(LF, LFT, tsp, (servoHome[LFT] - tms), 1, 0);

  update_sequencer(RR, RRF, fsp, (servoHome[RRF] - fms), 0, 0);
  update_sequencer(RR, RRT, tsp, (servoHome[RRT] + tms), 1, 0);
  update_sequencer(LR, LRF, fsp, (servoHome[LRF] + fms), 0, 0);
  update_sequencer(LR, LRT, tsp, (servoHome[LRT] - tms), 1, 0);

  move_kin_x = 0;

  lastMoveDelayUpdate = millis();  
}


void move_ky() {

  int fms = (move_steps_ky * 0.8);
  int tms = (move_steps_ky * 1.3);
  int fsp = limit_speed((24 * spd_factor));
  int tsp = limit_speed((14 * spd_factor));

  update_sequencer(RF, RFF, fsp, (servoHome[RFF] - fms), 0, 0);
  update_sequencer(RF, RFT, tsp, (servoHome[RFT] + tms), 1, 0);
  update_sequencer(LF, LFF, fsp, (servoHome[LFF] + fms), 0, 0);
  update_sequencer(LF, LFT, tsp, (servoHome[LFT] - tms), 1, 0);

  update_sequencer(RR, RRF, fsp, (servoHome[RRF] - fms), 0, 0);
  update_sequencer(RR, RRT, tsp, (servoHome[RRT] + tms), 1, 0);
  update_sequencer(LR, LRF, fsp, (servoHome[LRF] + fms), 0, 0);
  update_sequencer(LR, LRT, tsp, (servoHome[LRT] - tms), 1, 0);

  move_kin_y = 0;

  lastMoveDelayUpdate = millis(); 
}


void roll_x() {
  update_sequencer(LF, LFC, spd, (servoHome[LFC] + move_steps_x), 0, 0);
  update_sequencer(LR, LRC, spd, (servoHome[LRC] + move_steps_x), 0, 0);
  update_sequencer(RF, RFC, spd, (servoHome[RFC] + move_steps_x), 0, 0);
  update_sequencer(RR, RRC, spd, (servoHome[RRC] + move_steps_x), 0, 0);

  move_roll_x = 0;

  lastMoveDelayUpdate = millis();  
}


void pitch_y() {

  int fms = (move_steps_y * 0.4);
  int tms = (move_steps_y * 0.65);
  int fsp = limit_speed((24 * spd_factor));
  int tsp = limit_speed((14 * spd_factor));

  update_sequencer(RF, RFF, fsp, (servoHome[RFF] - fms), 0, 0);
  update_sequencer(RF, RFT, tsp, (servoHome[RFT] + tms), 1, 0);
  update_sequencer(LF, LFF, fsp, (servoHome[LFF] + fms), 0, 0);
  update_sequencer(LF, LFT, tsp, (servoHome[LFT] - tms), 1, 0);

  update_sequencer(RR, RRF, fsp, (servoHome[RRF] + fms), 0, 0);
  update_sequencer(RR, RRT, tsp, (servoHome[RRT] - tms), 1, 0);
  update_sequencer(LR, LRF, fsp, (servoHome[LRF] - fms), 0, 0);
  update_sequencer(LR, LRT, tsp, (servoHome[LRT] + tms), 1, 0);

  move_pitch_y = 0;

  lastMoveDelayUpdate = millis();  
}


void yaw() {
  int cms = (move_steps_yaw * 0.4);
  int fms = (move_steps_yaw * 0.3);
  int tms = (move_steps_yaw * 0.1);
  int csp = limit_speed((10 * spd_factor));
  int fsp = limit_speed((20 * spd_factor));
  int tsp = limit_speed((40 * spd_factor));

//  int lfms = fms;
//  int rfms = fms;
  int ltms = tms;
  int rtms = tms;
  if (move_steps_yaw < 0) {
//    lfms = (move_steps_yaw * 0.5);
    ltms = (move_steps_yaw * 0.4);
  } else {
//    rfms = (move_steps_yaw * 0.5);
    rtms = (move_steps_yaw * 0.4);
  }

  int lfsp = fsp;
  int rfsp = fsp;
  int ltsp = tsp;
  int rtsp = tsp;
  if (move_steps_yaw < 0) {
    lfsp = limit_speed((8 * spd_factor));
    ltsp = limit_speed((10 * spd_factor));
  } else {
    rfsp = limit_speed((8 * spd_factor));
    rtsp = limit_speed((10 * spd_factor));
  }

  update_sequencer(LF, LFC, csp, (servoHome[LFC] + cms), 0, 0);
  update_sequencer(LF, LFF, lfsp, (servoHome[LFF] + fms), 1, 0);
  update_sequencer(LF, LFT, ltsp, (servoHome[LFT] - ltms), 1, 0);

  update_sequencer(RF, RFC, csp, (servoHome[RFC] + cms), 0, 0);
  update_sequencer(RF, RFF, rfsp, (servoHome[RFF] + fms), 1, 0);
  update_sequencer(RF, RFT, rtsp, (servoHome[RFT] - rtms), 1, 0);

  update_sequencer(LR, LRC, csp, (servoHome[LRC] - cms), 0, 0);
  update_sequencer(LR, LRF, lfsp, (servoHome[LRF] - fms), 1, 0);
  update_sequencer(LR, LRT, ltsp, (servoHome[LRT] + ltms), 1, 0);

  update_sequencer(RR, RRC, csp, (servoHome[RRC] - cms), 0, 0);
  update_sequencer(RR, RRF, rfsp, (servoHome[RRF] - fms), 1, 0);
  update_sequencer(RR, RRT, rtsp, (servoHome[RRT] + rtms), 1, 0);

  move_yaw = 0;

  lastMoveDelayUpdate = millis();  
}


void yaw_x() {
  int cms = (move_steps_yaw_x * 0.9);
  int fms = (move_steps_yaw_x * 0.4);
  int tms = (move_steps_yaw_x * 0.7);
  int csp = limit_speed((20 * spd_factor));
  int fsp = limit_speed((32 * spd_factor));
  int tsp = limit_speed((20 * spd_factor));

  update_sequencer(LF, LFC, csp, (servoHome[LFC] + cms), 0, 0);
  update_sequencer(LF, LFF, fsp, (servoHome[LFF] + fms), 1, 0);
  update_sequencer(LF, LFT, tsp, (servoHome[LFT] - tms), 1, 0);

  update_sequencer(LR, LRC, csp, (servoHome[LRC] + cms), 0, 0);
  update_sequencer(LR, LRF, fsp, (servoHome[LRF] + fms), 1, 0);
  update_sequencer(LR, LRT, tsp, (servoHome[LRT] - tms), 1, 0);

  update_sequencer(RF, RFC, csp, (servoHome[RFC] + cms), 0, 0);
  update_sequencer(RF, RFF, fsp, (servoHome[RFF] + fms), 1, 0);
  update_sequencer(RF, RFT, tsp, (servoHome[RFT] - tms), 1, 0);

  update_sequencer(RR, RRC, csp, (servoHome[RRC] + cms), 0, 0);
  update_sequencer(RR, RRF, fsp, (servoHome[RRF] + fms), 1, 0);
  update_sequencer(RR, RRT, tsp, (servoHome[RRT] - tms), 1, 0);

  move_yaw_x = 0;

  lastMoveDelayUpdate = millis();  
}


void yaw_y() {

  int fms = (move_steps_yaw_y * 0.6);
  int tms = (move_steps_yaw_y * 0.2);
  int fsp = limit_speed((25 * spd_factor));
  int tsp = limit_speed((30 * spd_factor));

  int ftms = tms;
  if (move_steps_yaw_y > 0) {
    ftms = (move_steps_yaw_y * 0.6);
  }

  update_sequencer(RF, RFF, fsp, (servoHome[RFF] - fms), 0, 0);
  update_sequencer(RF, RFT, tsp, (servoHome[RFT] - ftms), 1, 0);
  update_sequencer(LF, LFF, fsp, (servoHome[LFF] + fms), 0, 0);
  update_sequencer(LF, LFT, tsp, (servoHome[LFT] + ftms), 1, 0);

  update_sequencer(RR, RRF, fsp, (servoHome[RRF] - fms), 0, 0);
  update_sequencer(RR, RRT, tsp, (servoHome[RRT] - tms), 1, 0);
  update_sequencer(LR, LRF, fsp, (servoHome[LRF] + fms), 0, 0);
  update_sequencer(LR, LRT, tsp, (servoHome[LRT] + tms), 1, 0);

  move_yaw_y = 0;

  lastMoveDelayUpdate = millis();  
}


void x_axis() {
  if (!activeSweep[RRT]) {
    set_sweep(LFF, limit_speed((10 * spd_factor)),
              limit_target(LFF, ((servoHome[LFF] - 70) + (move_steps * .7)), 0),
              limit_target(LFF, ((servoHome[LFF] - 10) + (move_steps * .1)), 0), 1);

    set_sweep(LFT, limit_speed((10 * spd_factor)),
              limit_target(LFT, ((servoHome[LFT] + 85) - (move_steps * .85)), 0),
              limit_target(LFT, ((servoHome[LFT] + 25) - (move_steps * .25)), 0), 1);


    servoSpeed[RFF] = limit_speed((10 * spd_factor));
    //trailing values are the pre-tuning offsets, kept for reference: 70 was 65, 10 was 5
    set_sweep(RFF, servoSpeed[RFF],
              limit_target(RFF, ((servoHome[RFF] + 70) - (move_steps * .7)), 0),
              limit_target(RFF, ((servoHome[RFF] + 10) - (move_steps * .1)), 0), 1);

    set_sweep(RFT, limit_speed((10 * spd_factor)),
              limit_target(RFT, ((servoHome[RFT] - 85) + (move_steps * .85)), 0),
              limit_target(RFT, ((servoHome[RFT] - 25) + (move_steps * .25)), 0), 1);


    set_sweep(LRF, limit_speed((10 * spd_factor)),
              limit_target(LRF, ((servoHome[LRF] - 70) + (move_steps * .7)), 0),
              limit_target(LRF, ((servoHome[LRF] - 10) + (move_steps * .1)), 0), 1);

    servoSpeed[LRT] = limit_speed((10 * spd_factor));
    //trailing values are the pre-tuning offsets, kept for reference: 85 was 90, 25 was 30
    set_sweep(LRT, servoSpeed[LRT],
              limit_target(LRT, ((servoHome[LRT] + 85) - (move_steps * .85)), 0),
              limit_target(LRT, ((servoHome[LRT] + 25) - (move_steps * .25)), 0), 1);


    set_sweep(RRF, limit_speed((10 * spd_factor)),
              limit_target(RRF, ((servoHome[RRF] + 70) - (move_steps * .7)), 0),
              limit_target(RRF, ((servoHome[RRF] + 10) - (move_steps * .1)), 0), 1);

    set_sweep(RRT, limit_speed((10 * spd_factor)),
              limit_target(RRT, ((servoHome[RRT] - 85) + (move_steps * .85)), 0),
              limit_target(RRT, ((servoHome[RRT] - 25) + (move_steps * .25)), 0), 1);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_x_axis = 0;
      }
    }

    lastMoveDelayUpdate = millis();  
  }
}


void y_axis() {

  if (!activeSweep[RRT]) {
    set_sweep(LFC, limit_speed((96 * spd_factor)),
              servoHome[LFC],
              limit_target(LFC, (servoHome[LFC] + (move_steps * .05)), 0), 1);

    set_sweep(LFT, limit_speed((7 * spd_factor)),
              limit_target(LFT, (servoHome[LFT] - (move_steps * .65)), 0),
              limit_target(LFT, (servoHome[LFT] + (move_steps * .65)), 0), 1);

    set_sweep(LFF, limit_speed((12 * spd_factor)),
              limit_target(LFF, (servoHome[LFF] + (move_steps * .4)), 0),
              limit_target(LFF, (servoHome[LFF] - (move_steps * .4)), 0), 1);


    set_sweep(RFC, limit_speed((96 * spd_factor)),
              servoHome[RFC],
              limit_target(RFC, (servoHome[RFC] - (move_steps * .05)), 0), 1);

    set_sweep(RFT, limit_speed((7 * spd_factor)),
              limit_target(RFT, (servoHome[RFT] + (move_steps * .65)), 0),
              limit_target(RFT, (servoHome[RFT] - (move_steps * .65)), 0), 1);

    set_sweep(RFF, limit_speed((12 * spd_factor)),
              limit_target(RFF, (servoHome[RFF] - (move_steps * .4)), 0),
              limit_target(RFF, (servoHome[RFF] + (move_steps * .4)), 0), 1);


    set_sweep(LRC, limit_speed((96 * spd_factor)),
              servoHome[LRC],
              limit_target(LRC, (servoHome[LRC] + (move_steps * .05)), 0), 1);

    set_sweep(LRT, limit_speed((7 * spd_factor)),
              limit_target(LRT, (servoHome[LRT] - (move_steps * .65)), 0),
              limit_target(LRT, (servoHome[LRT] + (move_steps * .65)), 0), 1);

    set_sweep(LRF, limit_speed((12 * spd_factor)),
              limit_target(LRF, (servoHome[LRF] + (move_steps * .4)), 0),
              limit_target(LRF, (servoHome[LRF] - (move_steps * .4)), 0), 1);


    set_sweep(RRC, limit_speed((96 * spd_factor)),
              servoHome[RRC],
              limit_target(RRC, (servoHome[RRC] - (move_steps * .05)), 0), 1);

    set_sweep(RRT, limit_speed((7 * spd_factor)),
              limit_target(RRT, (servoHome[RRT] + (move_steps * .65)), 0),
              limit_target(RRT, (servoHome[RRT] - (move_steps * .65)), 0), 1);

    set_sweep(RRF, limit_speed((12 * spd_factor)),
              limit_target(RRF, (servoHome[RRF] - (move_steps * .4)), 0),
              limit_target(RRF, (servoHome[RRF] + (move_steps * .4)), 0), 1);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_y_axis = 0;
      }
    }
  
    lastMoveDelayUpdate = millis();  
  }
}


void roll() {
  if (!activeSweep[RRF]) {
    set_sweep(LFT, limit_speed((10 * spd_factor)),
              limit_target(LFT, (servoHome[LFT] - move_steps), 0),
              limit_target(LFT, (servoHome[LFT] + move_steps), 0), 1);

    set_sweep(LFF, limit_speed((16 * spd_factor)),
              limit_target(LFF, (servoHome[LFF] + (move_steps * .667)), 0),
              limit_target(LFF, (servoHome[LFF] - (move_steps * .667)), 0), 1);


    set_sweep(LRT, limit_speed((10 * spd_factor)),
              limit_target(LRT, (servoHome[LRT] - move_steps), 0),
              limit_target(LRT, (servoHome[LRT] + move_steps), 0), 1);

    set_sweep(LRF, limit_speed((16 * spd_factor)),
              limit_target(LRF, (servoHome[LRF] + (move_steps * .667)), 0),
              limit_target(LRF, (servoHome[LRF] - (move_steps * .667)), 0), 1);


    set_sweep(RFT, limit_speed((10 * spd_factor)),
              limit_target(RFT, (servoHome[RFT] - move_steps), 0),
              limit_target(RFT, (servoHome[RFT] + move_steps), 0), 1);

    set_sweep(RFF, limit_speed((16 * spd_factor)),
              limit_target(RFF, (servoHome[RFF] + (move_steps * .667)), 0),
              limit_target(RFF, (servoHome[RFF] - (move_steps * .667)), 0), 1);


    set_sweep(RRT, limit_speed((10 * spd_factor)),
              limit_target(RRT, (servoHome[RRT] - move_steps), 0),
              limit_target(RRT, (servoHome[RRT] + move_steps), 0), 1);

    set_sweep(RRF, limit_speed((16 * spd_factor)),
              limit_target(RRF, (servoHome[RRF] + (move_steps * .667)), 0),
              limit_target(RRF, (servoHome[RRF] - (move_steps * .667)), 0), 1);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_roll = 0;
      }
    }

    lastMoveDelayUpdate = millis();  
  }
}


void roll_body() {
  if (!activeSweep[RRC]) {
    set_sweep(LFC, limit_speed((10 * spd_factor)),
              limit_target(LFC, (servoHome[LFC] - move_steps), 0),
              limit_target(LFC, (servoHome[LFC] + move_steps), 0), 1);

    set_sweep(LRC, limit_speed((10 * spd_factor)),
              limit_target(LRC, (servoHome[LRC] - move_steps), 0),
              limit_target(LRC, (servoHome[LRC] + move_steps), 0), 1);

    set_sweep(RFC, limit_speed((10 * spd_factor)),
              limit_target(RFC, (servoHome[RFC] - move_steps), 0),
              limit_target(RFC, (servoHome[RFC] + move_steps), 0), 1);

    set_sweep(RRC, limit_speed((10 * spd_factor)),
              limit_target(RRC, (servoHome[RRC] - move_steps), 0),
              limit_target(RRC, (servoHome[RRC] + move_steps), 0), 1);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_roll_body = 0;
      }
    }

    lastMoveDelayUpdate = millis();  
  }
}


void pitch(int xdir) {
  float sinc0 = .15;
  float sinc1 = 1.15;
  float sweep_lo, sweep_hi;   //scratch for the per-tibia sweep endpoints below

  if (xdir < 0) {  //turn left
    xdir = abs(xdir);
    servoStepMoves[RFC][0] = limit_target(RFC, (servoPos[RFC] + (xdir / 4)), 25);
    servoStepMoves[RRC][0] = limit_target(RRC, (servoPos[RRC] + (xdir / 4)), 25);
    servoStepMoves[LFC][0] = 0;
    servoStepMoves[LRC][0] = 0;
  } else if (xdir > 0) {  //turn right
    servoStepMoves[RFC][0] = 0;
    servoStepMoves[RRC][0] = 0;
    servoStepMoves[LFC][0] = limit_target(LFC, (servoPos[LFC] + (xdir / 4)), 25);
    servoStepMoves[LRC][0] = limit_target(LRC, (servoPos[LRC] + (xdir / 4)), 25);
  } else {
    servoStepMoves[RFC][0] = 0;
    servoStepMoves[RRC][0] = 0;
    servoStepMoves[LFC][0] = 0;
    servoStepMoves[LRC][0] = 0;
  }

  if (!activeSweep[RRT]) {
    //a negative move_steps mirrors the sweep, so the pitch reverses direction
    sweep_lo = (move_steps < 0)
      ? limit_target(LFT, (servoHome[LFT] + (move_steps * sinc0)), 0)
      : limit_target(LFT, (servoHome[LFT] - (move_steps * sinc0)), 0);
    sweep_hi = (move_steps < 0)
      ? limit_target(LFT, (servoHome[LFT] - (move_steps * sinc1)), 0)
      : limit_target(LFT, (servoHome[LFT] + (move_steps * sinc1)), 0);
    set_sweep(LFT, limit_speed((7 * spd_factor)), sweep_lo, sweep_hi, 1);

    //a negative move_steps mirrors the sweep, so the pitch reverses direction
    sweep_lo = (move_steps < 0)
      ? limit_target(RFT, (servoHome[RFT] - (move_steps * sinc0)), 0)
      : limit_target(RFT, (servoHome[RFT] + (move_steps * sinc0)), 0);
    sweep_hi = (move_steps < 0)
      ? limit_target(RFT, (servoHome[RFT] + (move_steps * sinc1)), 0)
      : limit_target(RFT, (servoHome[RFT] - (move_steps * sinc1)), 0);
    set_sweep(RFT, limit_speed((7 * spd_factor)), sweep_lo, sweep_hi, 1);

    //a negative move_steps mirrors the sweep, so the pitch reverses direction
    sweep_lo = (move_steps < 0)
      ? limit_target(LRT, (servoHome[LRT] - (move_steps * sinc0)), 0)
      : limit_target(LRT, (servoHome[LRT] + (move_steps * sinc0)), 0);
    sweep_hi = (move_steps < 0)
      ? limit_target(LRT, (servoHome[LRT] + (move_steps * sinc1)), 0)
      : limit_target(LRT, (servoHome[LRT] - (move_steps * sinc1)), 0);
    set_sweep(LRT, limit_speed((7 * spd_factor)), sweep_lo, sweep_hi, 1);

    //a negative move_steps mirrors the sweep, so the pitch reverses direction
    sweep_lo = (move_steps < 0)
      ? limit_target(RRT, (servoHome[RRT] + (move_steps * sinc0)), 0)
      : limit_target(RRT, (servoHome[RRT] - (move_steps * sinc0)), 0);
    sweep_hi = (move_steps < 0)
      ? limit_target(RRT, (servoHome[RRT] - (move_steps * sinc1)), 0)
      : limit_target(RRT, (servoHome[RRT] + (move_steps * sinc1)), 0);
    set_sweep(RRT, limit_speed((7 * spd_factor)), sweep_lo, sweep_hi, 1);

    update_sequencer(LF, LFC, limit_speed((7 * spd_factor)), servoStepMoves[LFC][0], 0, 0);
    update_sequencer(RF, RFC, limit_speed((7 * spd_factor)), servoStepMoves[RFC][0], 0, 0);
    update_sequencer(LR, LRC, limit_speed((7 * spd_factor)), servoStepMoves[LRC][0], 0, 0);
    update_sequencer(RR, RRC, limit_speed((7 * spd_factor)), servoStepMoves[RRC][0], 0, 0);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_pitch = 0;
      }
    }

    lastMoveDelayUpdate = millis();  
  }
}


void pitch_body() {
  if (!activeSweep[RRF]) {
    set_sweep(LFC, limit_speed((68 * spd_factor)),
              servoHome[LFC],
              limit_target(LFC, (servoHome[LFC] + (move_steps * .05)), 0), 1);

    set_sweep(LFT, limit_speed((10 * spd_factor)),
              limit_target(LFT, (servoHome[LFT] - (move_steps * .35)), 0),
              limit_target(LFT, (servoHome[LFT] + (move_steps * .35)), 0), 1);

    set_sweep(LFF, limit_speed((17 * spd_factor)),
              limit_target(LFF, (servoHome[LFF] + (move_steps * .2)), 0),
              limit_target(LFF, (servoHome[LFF] - (move_steps * .2)), 0), 1);


    set_sweep(RFC, limit_speed((68 * spd_factor)),
              servoHome[RFC],
              limit_target(RFC, (servoHome[RFC] - (move_steps * .05)), 0), 1);

    set_sweep(RFT, limit_speed((10 * spd_factor)),
              limit_target(RFT, (servoHome[RFT] + (move_steps * .35)), 0),
              limit_target(RFT, (servoHome[RFT] - (move_steps * .35)), 0), 1);

    set_sweep(RFF, limit_speed((17 * spd_factor)),
              limit_target(RFF, (servoHome[RFF] - (move_steps * .2)), 0),
              limit_target(RFF, (servoHome[RFF] + (move_steps * .2)), 0), 1);


    set_sweep(LRC, limit_speed((68 * spd_factor)),
              limit_target(LRC, (servoHome[LRC] - (move_steps * .05)), 0),
              servoHome[LRC], 1);

    set_sweep(LRT, limit_speed((10 * spd_factor)),
              limit_target(LRT, (servoHome[LRT] + (move_steps * .35)), 0),
              limit_target(LRT, (servoHome[LRT] - (move_steps * .35)), 0), 1);

    set_sweep(LRF, limit_speed((17 * spd_factor)),
              limit_target(LRF, (servoHome[LRF] - (move_steps * .2)), 0),
              limit_target(LRF, (servoHome[LRF] + (move_steps * .2)), 0), 1);


    set_sweep(RRC, limit_speed((68 * spd_factor)),
              limit_target(RRC, (servoHome[RRC] + (move_steps * .05)), 0),
              servoHome[RRC], 1);

    set_sweep(RRT, limit_speed((10 * spd_factor)),
              limit_target(RRT, (servoHome[RRT] - (move_steps * .35)), 0),
              limit_target(RRT, (servoHome[RRT] + (move_steps * .35)), 0), 1);

    set_sweep(RRF, limit_speed((17 * spd_factor)),
              limit_target(RRF, (servoHome[RRF] + (move_steps * .2)), 0),
              limit_target(RRF, (servoHome[RRF] - (move_steps * .2)), 0), 1);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_pitch_body = 0;
      }
    }
  
    lastMoveDelayUpdate = millis();  
  }
}
