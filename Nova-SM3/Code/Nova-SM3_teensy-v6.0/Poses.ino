/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Named stances
 *
 *   Fixed poses. Each simply points every servo at a position derived from
 *   its home or its travel limit and lets AsyncServo take it there, so they
 *   return immediately rather than waiting for the pose to be reached.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   Position Functions
   -------------------------------------------------------
*/
void set_stay() {
  for (int m = 0; m < TOTAL_SERVOS; m++) {
    activeSweep[m] = 0;
    activeServo[m] = 1;
    targetPos[m] = servoHome[m];
    servoSpeed[m] = 10;
    if (is_tibia(m)) {
      servoSpeed[m] = 5;
    }
  }

  lastMoveDelayUpdate = millis();
}


void set_sit() {
  activeServo[LFC] = 1;
  servoSpeed[LFC] = 10;
  targetPos[LFC] = servoHome[LFC];
  activeServo[LRC] = 1;
  servoSpeed[LRC] = 10;
  targetPos[LRC] = servoHome[LRC];
  activeServo[RFC] = 1;
  servoSpeed[RFC] = 10;
  targetPos[RFC] = servoHome[RFC];
  activeServo[RRC] = 1;
  servoSpeed[RRC] = 10;
  targetPos[RRC] = servoHome[RRC];

  activeServo[LFT] = 1;
  servoSpeed[LFT] = 10;
  targetPos[LFT] = servoLimit[LFT][0];
  activeServo[RFT] = 1;
  servoSpeed[RFT] = 10;
  targetPos[RFT] = servoLimit[RFT][0];

  activeServo[LRT] = 1;
  servoSpeed[LRT] = 10;
  targetPos[LRT] = servoLimit[LRT][1];
  activeServo[RRT] = 1;
  servoSpeed[RRT] = 10;
  targetPos[RRT] = servoLimit[RRT][1];

  activeServo[LRF] = 1;
  servoSpeed[LRF] = 10;
  targetPos[LRF] = (servoLimit[LRF][0] - 30);
  activeServo[RRF] = 1;
  servoSpeed[RRF] = 10;
  targetPos[RRF] = (servoLimit[RRF][0] + 30);

  activeServo[LFF] = 1;
  servoSpeed[LFF] = 20;
  targetPos[LFF] = (servoLimit[LFF][1] + 90);
  activeServo[RFF] = 1;
  servoSpeed[RFF] = 20;
  targetPos[RFF] = (servoLimit[RFF][1] - 90);

  lastMoveDelayUpdate = millis();
}


void set_crouch() {
  activeServo[LFC] = 1;
  servoSpeed[LFC] = 10;
  targetPos[LFC] = servoHome[LFC];
  activeServo[LRC] = 1;
  servoSpeed[LRC] = 10;
  targetPos[LRC] = servoHome[LRC];
  activeServo[RFC] = 1;
  servoSpeed[RFC] = 10;
  targetPos[RFC] = servoHome[RFC];
  activeServo[RRC] = 1;
  servoSpeed[RRC] = 10;
  targetPos[RRC] = servoHome[RRC];

  activeServo[LFT] = 1;
  servoSpeed[LFT] = 10;
  targetPos[LFT] = servoLimit[LFT][1];
  activeServo[RFT] = 1;
  servoSpeed[RFT] = 10;
  targetPos[RFT] = servoLimit[RFT][1];

  activeServo[LRT] = 1;
  servoSpeed[LRT] = 10;
  targetPos[LRT] = servoLimit[LRT][1];
  activeServo[RRT] = 1;
  servoSpeed[RRT] = 10;
  targetPos[RRT] = servoLimit[RRT][1];

  activeServo[LRF] = 1;
  servoSpeed[LRF] = 10;
  targetPos[LRF] = (servoLimit[LRF][0] - 30);
  activeServo[RRF] = 1;
  servoSpeed[RRF] = 10;
  targetPos[RRF] = (servoLimit[RRF][0] + 30);

  activeServo[LFF] = 1;
  servoSpeed[LFF] = 20;
  targetPos[LFF] = (servoLimit[LFF][0] - 30);
  activeServo[RFF] = 1;
  servoSpeed[RFF] = 20;
  targetPos[RFF] = (servoLimit[RFF][0] + 30);

  lastMoveDelayUpdate = millis();
}


void set_lay() {
  activeServo[LFC] = 1;
  servoSpeed[LFC] = 20;
  targetPos[LFC] = (servoLimit[LFC][1]);
  activeServo[LRC] = 1;
  servoSpeed[LRC] = 20;
  targetPos[LRC] = (servoLimit[LRC][1]);
  activeServo[RFC] = 1;
  servoSpeed[RFC] = 20;
  targetPos[RFC] = (servoLimit[RFC][1]);
  activeServo[RRC] = 1;
  servoSpeed[RRC] = 20;
  targetPos[RRC] = (servoLimit[RRC][1]);

  activeServo[LFT] = 1;
  servoSpeed[LFT] = 10;
  targetPos[LFT] = servoLimit[LFT][1];
  activeServo[RFT] = 1;
  servoSpeed[RFT] = 10;
  targetPos[RFT] = servoLimit[RFT][1];

  activeServo[LRT] = 1;
  servoSpeed[LRT] = 10;
  targetPos[LRT] = servoLimit[LRT][1];
  activeServo[RRT] = 1;
  servoSpeed[RRT] = 10;
  targetPos[RRT] = servoLimit[RRT][1];

  activeServo[LRF] = 1;
  servoSpeed[LRF] = 10;
  targetPos[LRF] = (servoLimit[LRF][0]);
  activeServo[RRF] = 1;
  servoSpeed[RRF] = 10;
  targetPos[RRF] = (servoLimit[RRF][0]);

  activeServo[LFF] = 1;
  servoSpeed[LFF] = 20;
  targetPos[LFF] = (servoLimit[LFF][0]);
  activeServo[RFF] = 1;
  servoSpeed[RFF] = 20;
  targetPos[RFF] = (servoLimit[RFF][0]);

  lastMoveDelayUpdate = millis();
}


void set_kneel() {
  activeServo[LFC] = 1;
  servoSpeed[LFC] = 10;
  targetPos[LFC] = (servoLimit[LFC][0] - 10);
  activeServo[LRC] = 1;
  servoSpeed[LRC] = 10;
  targetPos[LRC] = (servoLimit[LRC][0] - 10);
  activeServo[RFC] = 1;
  servoSpeed[RFC] = 10;
  targetPos[RFC] = (servoLimit[RFC][0] + 10);
  activeServo[RRC] = 1;
  servoSpeed[RRC] = 10;
  targetPos[RRC] = (servoLimit[RRC][0] + 10);

  activeServo[LFT] = 1;
  servoSpeed[LFT] = 20;
  targetPos[LFT] = (servoHome[LFT] - 40);
  activeServo[RFT] = 1;
  servoSpeed[RFT] = 20;
  targetPos[RFT] = (servoHome[RFT] + 40);

  activeServo[LRT] = 1;
  servoSpeed[LRT] = 20;
  targetPos[LRT] = (servoHome[LRT] - 40);
  activeServo[RRT] = 1;
  servoSpeed[RRT] = 20;
  targetPos[RRT] = (servoHome[RRT] + 40);

  activeServo[LRF] = 1;
  servoSpeed[LRF] = 10;
  targetPos[LRF] = (servoLimit[LRF][1] + 90);
  activeServo[RRF] = 1;
  servoSpeed[RRF] = 10;
  targetPos[RRF] = (servoLimit[RRF][1] - 90);

  activeServo[LFF] = 1;
  servoSpeed[LFF] = 10;
  targetPos[LFF] = (servoLimit[LFF][1] + 90);
  activeServo[RFF] = 1;
  servoSpeed[RFF] = 10;
  targetPos[RFF] = (servoLimit[RFF][1] - 90);

  lastMoveDelayUpdate = millis();
}


void look_left() {
  if (rgb_active) {
    rgb_request(RGB_LEFT RGB_PURPLE RGB_RIGHT RGB_PURPLE RGB_COUNT_3 RGB_SPEED_100 RGB_PATTERN_BLINK);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    update_sequencer(LF, LFT, spd, (servoHome[LFT] + move_steps), 1, 0);
    update_sequencer(LF, LFC, spd, (servoHome[LFC] + move_steps), 1, 0);
    update_sequencer(LR, LRT, spd, (servoHome[LRT] + move_steps), 1, 0);
    update_sequencer(LR, LRC, spd, (servoHome[LRC] + move_steps), 1, 0);
    update_sequencer(RR, RRT, spd, (servoHome[RRT] + move_steps), 1, 0);
    update_sequencer(RR, RRC, spd, (servoHome[RRC] + move_steps), 1, 0);
    update_sequencer(RF, RFT, spd, (servoHome[RFT] + move_steps), 1, 0);
    update_sequencer(RF, RFC, spd, (servoHome[RFC] + move_steps), 1, 0);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 1) {
    update_sequencer(RF, RFC, spd, servoHome[RFC], 2, 1000);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (is_left_leg(i) && !is_front_leg(i)) {
        update_sequencer(LR, i, spd, servoHome[i], 3, 0);
      } else if (is_left_leg(i) && is_front_leg(i)) {
        update_sequencer(LF, i, spd, servoHome[i], 3, 0);
      } else if (!is_left_leg(i) && is_front_leg(i)) {
        update_sequencer(RF, i, spd, servoHome[i], 3, 0);
      } else {
        update_sequencer(RR, i, spd, servoHome[i], 3, 0);
      }
    }
  }
  
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 3) {
    update_sequencer(RF, RFC, spd, servoHome[RFC], 4, 1000);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 4) {
    for (int i = 0; i < TOTAL_LEGS; i++) {
      servoSequence[i] = 0;
    }
    
    move_look_left = 0;
    move_loops = 4;
    move_switch = 2;
    move_wake = 1;

    //if called from follow routine, restore
    if (move_paused == "follow") {
      move_follow = 1;
      spd = spd_prev;
      set_speed();
    }
  }
}


void look_right() {
  if (rgb_active) {
    rgb_request(RGB_LEFT RGB_PURPLE RGB_RIGHT RGB_PURPLE RGB_COUNT_3 RGB_SPEED_100 RGB_PATTERN_BLINK);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    update_sequencer(LF, LFT, spd, (servoHome[LFT] - move_steps), 1, 0);
    update_sequencer(LF, LFC, spd, (servoHome[LFC] - move_steps), 1, 0);
    update_sequencer(LR, LRT, spd, (servoHome[LRT] - move_steps), 1, 0);
    update_sequencer(LR, LRC, spd, (servoHome[LRC] - move_steps), 1, 0);
    update_sequencer(RR, RRT, spd, (servoHome[RRT] - move_steps), 1, 0);
    update_sequencer(RR, RRC, spd, (servoHome[RRC] - move_steps), 1, 0);
    update_sequencer(RF, RFT, spd, (servoHome[RFT] - move_steps), 1, 0);
    update_sequencer(RF, RFC, spd, (servoHome[RFC] - move_steps), 1, 0);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 1) {
    update_sequencer(RF, RFC, spd, servoHome[RFC], 2, 1000);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (is_left_leg(i) && !is_front_leg(i)) {
        update_sequencer(LR, i, spd, servoHome[i], 3, 0);
      } else if (is_left_leg(i) && is_front_leg(i)) {
        update_sequencer(LF, i, spd, servoHome[i], 3, 0);
      } else if (!is_left_leg(i) && is_front_leg(i)) {
        update_sequencer(RF, i, spd, servoHome[i], 3, 0);
      } else {
        update_sequencer(RR, i, spd, servoHome[i], 3, 0);
      }
    }
  }
  
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 3) {
    update_sequencer(RF, RFC, spd, servoHome[RFC], 4, 1000);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 4) {
    for (int i = 0; i < TOTAL_LEGS; i++) {
      servoSequence[i] = 0;
    }
    
    move_look_right = 0;
    move_loops = 4;
    move_switch = 2;
    move_wake = 1;

    //if called from follow routine, restore
    if (move_paused == "follow") {
      move_follow = 1;
      spd = spd_prev;
      set_speed();
    }
  }
}
