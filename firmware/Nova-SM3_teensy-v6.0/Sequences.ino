/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Scripted routines and the sequencer
 *
 *   Multi-step routines, and update_sequencer() - the single function every
 *   movement routine uses to queue one servo move as part of a sequence.
 *   delay_sequences() is the timer that walks a scripted routine along.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

void run_demo() {
  if (!move_delay_sequences[0] && !move_delay_sequences[9]) {
    ramp_dist = 0.2;
    ramp_spd = 0.5;
    use_ramp = 1;

    move_demo = 1;

    move_delays[0] = 300;
    move_delay_sequences[0] = 15;
    move_delays[1] = 300;
    move_delay_sequences[1] = 14;
    move_delays[2] = 1200;
    move_delay_sequences[2] = 1;
    move_delays[3] = 3000;
    move_delay_sequences[3] = 2;
    move_delays[4] = 900;
    move_delay_sequences[4] = 3;
    move_delays[5] = 900;
    move_delay_sequences[5] = 5;
    move_delays[6] = 1500;
    move_delay_sequences[6] = 6;
    move_delays[7] = 1500;
    move_delay_sequences[7] = 7;
    move_delays[8] = 1500;
    move_delay_sequences[8] = 4;
    
    move_delays[9] = 900;
    move_delay_sequences[9] = 13;

    move_delays[10] = 1500;
    move_delay_sequences[10] = 11;
    
    move_delays[11] = 1500;
    move_delay_sequences[11] = 12;

    move_delays[12] = 3000;
    move_delay_sequences[12] = 8;
    move_delays[13] = 6000;
    move_delay_sequences[13] = 9;
    move_delays[14] = 3000;
    move_delay_sequences[14] = 10;

    delay_sequences();
  }
}


void funplay() {
  //recover from sitting up
  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && !servoSequence[RR]) {
    update_sequencer(RR, RRT, 8, servoLimit[RRT][1]-80, 1, 0);
    update_sequencer(LR, LRT, 8, servoLimit[LRT][1]+80, 1, 0);
  }
  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 1) {
    update_sequencer(RR, RRF, 6, servoPos[RRF]+30, 2, 300);
    update_sequencer(LR, LRF, 6, servoPos[LRF]-30, 2, 300);
  }
  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 2) {
///repeat/open step!
    update_sequencer(RR, RRT, 8, servoLimit[RRT][1]-80, 3, 0);
    update_sequencer(LR, LRT, 8, servoLimit[LRT][1]+80, 3, 0);
  }

  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 3) {
    update_sequencer(RR, RRT, 12, servoLimit[RRT][0], 4, 50);
    update_sequencer(LR, LRT, 12, servoLimit[LRT][0], 4, 50);

    update_sequencer(RF, RFF, 24, servoHome[RFF], 1, 50);
    update_sequencer(LF, LFF, 24, servoHome[LFF], 1, 50);
    update_sequencer(RF, RFT, 24, servoLimit[RFT][1], 1, 50);
    update_sequencer(LF, LFT, 24, servoLimit[LFT][1], 1, 50);
  }

  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 4) {
    set_sweep(RFF, 3, limit_target(RFF, (servoHome[RFF] + 40), 0), limit_target(RFF, (servoHome[RFF] + 80), 20), 9);

    set_sweep(LFF, 3, limit_target(LFF, (servoHome[LFF] - 80), 20), limit_target(LFF, (servoHome[LFF] - 40), 0), 9);

    set_sweep(RFT, 5, servoLimit[RFT][1], servoHome[RFT], 4);

    set_sweep(LFT, 5, servoHome[LFT], servoLimit[LFT][1], 4);

    update_sequencer(RR, RRC, 3, servoPos[RRC], 5, 0);
  }

  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 5 && !activeSweep[RFT] && !activeSweep[LFT]) {
    update_sequencer(RF, RFT, 12, servoHome[RFT], 2, 50);
    update_sequencer(LF, LFT, 12, servoHome[LFT], 2, 50);
    update_sequencer(RR, RRT, 12, servoHome[RRT], 6, 100);
    update_sequencer(LR, LRT, 12, servoHome[LRT], 6, 100);
    update_sequencer(RR, RRF, 12, servoPos[RRF]-10, 6, 0);
    update_sequencer(LR, LRF, 12, servoPos[LRF]+10, 6, 0);

    update_sequencer(RF, RFF, 6, servoLimit[RFF][1], 2, 200);
    update_sequencer(LF, LFF, 6, servoLimit[LFF][1], 2, 200);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
    update_sequencer(RF, RFC, 12, servoHome[RFC], 3, 0);
    update_sequencer(RR, RRF, 12, servoPos[RRF]+20, 7, 0);
    update_sequencer(LR, LRF, 12, servoPos[LRF]-20, 7, 0);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 3 && !activeServo[RRF] && !activeServo[LRF]) {
    update_sequencer(RR, RRT, 6, servoLimit[RRT][1], 8, 50);
    update_sequencer(LR, LRT, 6, servoLimit[LRT][1], 8, 50);
    update_sequencer(RF, RFF, 12, servoHome[RFF]+30, 4, 100);
    update_sequencer(LF, LFF, 12, servoHome[LFF]-30, 4, 100);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 4 && !activeServo[RRF] && !activeServo[LRF]) {
    update_sequencer(RF, RFC, 12, servoHome[RFC], 5, 0);
    update_sequencer(RR, RRT, 6, servoLimit[RRT][1], 9, 50);
    update_sequencer(LR, LRT, 6, servoLimit[LRT][1], 9, 50);
    update_sequencer(RR, RRF, 12, servoHome[RRF]-30, 9, 100);
    update_sequencer(LR, LRF, 12, servoHome[LRF]+30, 9, 100);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 5 && !activeServo[RRF] && !activeServo[LRF]) {
    update_sequencer(RR, RRT, 6, servoHome[RRT], 10, 0);
    update_sequencer(LR, LRT, 6, servoHome[LRT], 10, 0);
    update_sequencer(RF, RFT, 6, servoHome[RFT], 6, 30);
    update_sequencer(LF, LFT, 6, servoHome[LFT], 6, 30);
    update_sequencer(RR, RRF, 6, servoHome[RRF], 9, 100);
    update_sequencer(LR, LRF, 6, servoHome[LRF], 9, 100);
    update_sequencer(RF, RFF, 6, servoHome[RFF], 6, 130);
    update_sequencer(LF, LFF, 6, servoHome[LFF], 6, 130);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 6) {
    lastMoveDelayUpdate = millis();  

    if (rgb_active) {
      rgb_request(RGB_SPEED_250 RGB_RAINBOW);
    }
    if (buzz_active) {
      play_phrases();
    }
    move_funplay = 0;
    set_stop_active();
    delay(3000);
  }

}


/*
   -------------------------------------------------------
   Sequence Processing Functions
   -------------------------------------------------------
*/
void run_sequence() {

  //SEQ 1
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    update_sequencer(RF, RFC, spd_c, servoStepMoves[RFC][0], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFF, spd_f, servoStepMoves[RFF][0], servoSequence[RF], 0);
    update_sequencer(RF, RFT, spd_t, servoStepMoves[RFT][0], servoSequence[RF], 0);

    update_sequencer(LF, LFC, spd_c, servoStepMoves[LFC][0], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFF, spd_f, servoStepMoves[LFF][0], servoSequence[LF], 0);
    update_sequencer(LF, LFT, spd_t, servoStepMoves[LFT][0], servoSequence[LF], 0);

    update_sequencer(RR, RRC, spd_c, servoStepMoves[RRC][0], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRF, spd_f, servoStepMoves[RRF][0], servoSequence[RR], 0);
    update_sequencer(RR, RRT, spd_t, servoStepMoves[RRT][0], servoSequence[RR], 0);

    update_sequencer(LR, LRC, spd_c, servoStepMoves[LRC][0], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRF, spd_f, servoStepMoves[LRF][0], servoSequence[LR], 0);
    update_sequencer(LR, LRT, spd_t, servoStepMoves[LRT][0], servoSequence[LR], 0);
  }

  //SEQ 2
  if (!activeServo[LRC] && !activeServo[LRF] && !activeServo[LRT] && servoSequence[LR] == 1) {
    update_sequencer(RF, RFC, spd_c, servoStepMoves[RFC][1], (servoSequence[RF] + 1), move_delay);
    update_sequencer(RF, RFF, spd_f, servoStepMoves[RFF][1], servoSequence[RF], move_delay);
    update_sequencer(RF, RFT, spd_t, servoStepMoves[RFT][1], servoSequence[RF], move_delay);
  
    update_sequencer(LF, LFC, spd_c, servoStepMoves[LFC][1], (servoSequence[LF] + 1), move_delay);
    update_sequencer(LF, LFF, spd_f, servoStepMoves[LFF][1], servoSequence[LF], move_delay);
    update_sequencer(LF, LFT, spd_t, servoStepMoves[LFT][1], servoSequence[LF], move_delay);
  
    update_sequencer(RR, RRC, spd_c, servoStepMoves[RRC][1], (servoSequence[RR] + 1), move_delay);
    update_sequencer(RR, RRF, spd_f, servoStepMoves[RRF][1], servoSequence[RR], move_delay);
    update_sequencer(RR, RRT, spd_t, servoStepMoves[RRT][1], servoSequence[RR], move_delay);
  
    update_sequencer(LR, LRC, spd_c, servoStepMoves[LRC][1], (servoSequence[LR] + 1), move_delay);
    update_sequencer(LR, LRF, spd_f, servoStepMoves[LRF][1], servoSequence[LR], move_delay);
    update_sequencer(LR, LRT, spd_t, servoStepMoves[LRT][1], servoSequence[LR], move_delay);
  }

  //SEQ 3
  if (!activeServo[LRC] && !activeServo[LRF] && !activeServo[LRT] && servoSequence[LR] == 2) {
    update_sequencer(RF, RFC, spd_c, servoStepMoves[RFC][2], (servoSequence[RF] + 1), (move_delay*2));
    update_sequencer(RF, RFF, spd_f, servoStepMoves[RFF][2], servoSequence[RF], (move_delay*2));
    update_sequencer(RF, RFT, spd_t, servoStepMoves[RFT][2], servoSequence[RF], (move_delay*2));
  
    update_sequencer(LF, LFC, spd_c, servoStepMoves[LFC][2], (servoSequence[LF] + 1), (move_delay*2));
    update_sequencer(LF, LFF, spd_f, servoStepMoves[LFF][2], servoSequence[LF], (move_delay*2));
    update_sequencer(LF, LFT, spd_t, servoStepMoves[LFT][2], servoSequence[LF], (move_delay*2));
  
    update_sequencer(RR, RRC, spd_c, servoStepMoves[RRC][2], (servoSequence[RR] + 1), (move_delay*2));
    update_sequencer(RR, RRF, spd_f, servoStepMoves[RRF][2], servoSequence[RR], (move_delay*2));
    update_sequencer(RR, RRT, spd_t, servoStepMoves[RRT][2], servoSequence[RR], (move_delay*2));
  
    update_sequencer(LR, LRC, spd_c, servoStepMoves[LRC][2], (servoSequence[LR] + 1), (move_delay*2));
    update_sequencer(LR, LRF, spd_f, servoStepMoves[LRF][2], servoSequence[LR], (move_delay*2));
    update_sequencer(LR, LRT, spd_t, servoStepMoves[LRT][2], servoSequence[LR], (move_delay*2));
  }

  if (is_stepmove_complete(3) && servoSequence[LR] == 3) {
    if (move_loops) {
      move_loops--;
      //restart the sequence for the next loop. The original wrote indexes
      //0, l, 2 and 3 inside this loop, which cleared all four legs by
      //accident; clearing servoSequence[l] does exactly the same thing.
      for (int l = 0; l < TOTAL_LEGS; l++) {
        servoSequence[l] = 0;
      }
    } else {
      move_sequence = 0;
    }
  }
}


/*
   -------------------------------------------------------
   Delay Sequences
    :step a scripted routine along to its next movement

    A scripted routine (run_demo(), and the PIR alert recovery) is stored as
    two parallel arrays: move_delay_sequences[] says WHICH movement to run at
    each step, and move_delays[] says how long to wait before the step after
    it. This is called from loop() whenever moveDelayInterval has elapsed.

    Each call runs the first step still outstanding, clears it, and sets
    moveDelayInterval to that step's wait. When no steps are left, the eyes
    do a slow droop and the routine ends.

    The numbers are the routine ids used by move_delay_sequences[]. They are
    in numeric order below; the original chain listed 13 between 2 and 3,
    which made it easy to miss.
   -------------------------------------------------------
*/
void delay_sequences() {
  const int sequence_cnt = 16;
  int moved = 0;

  for (int i = 0; i < sequence_cnt; i++) {
    if (!move_delay_sequences[i]) continue;

    moved = 1;
    switch (move_delay_sequences[i]) {
      case 1:  //x-axis wiggle
        spd = 12;
        set_speed();
        move_loops = 6;
        move_steps = 20;
        move_x_axis = 1;
        if (debug1)
          Serial.print("move x");
        break;

      case 2:  //y-axis wiggle, large
        set_home();
        spd = 12;
        set_speed();
        move_loops = 3;
        move_steps = 70;
        move_y_axis = 1;
        if (debug1)
          Serial.print("move y large");
        break;

      case 3:  //body pitch
        spd = 9;
        set_speed();
        move_loops = 10;
        move_steps = 25;
        move_pitch_body = 1;
        if (debug1)
          Serial.print("move pitch_body");
        break;

      case 4:  //leg pitch
        use_ramp = 0;
        spd = 9;
        set_speed();
        move_loops = 10;
        move_steps = 25;
        move_pitch = 1;
        if (debug1)
          Serial.print("move pitch");
        break;

      case 5:  //body roll
        use_ramp = 1;
        spd = 9;
        set_speed();
        move_loops = 6;
        move_steps = 20;
        move_roll_body = 1;
        if (debug1)
          Serial.print("move rollb");
        break;

      case 6:  //leg roll
        set_home();
        spd = 5;
        set_speed();
        move_loops = 6;
        move_steps = 30;
        move_roll = 1;
        if (debug1)
          Serial.print("move roll");
        break;

      case 7:  //wake up
        set_home();
        spd = 1;
        set_speed();
        move_loops = 2;
        move_switch = 2;
        //snap the recorded positions to home first, so wake() lifts from
        //a known pose rather than wherever the last routine left the legs.
        //named srv, not i, so it cannot be mistaken for the sequence index
        for (int srv = 0; srv < TOTAL_SERVOS; srv++) {
          servoPos[srv] = servoHome[srv];
        }
        move_wake = 1;
        if (debug1)
          Serial.print("move wake");
        break;

      case 8:  //crouch
        set_crouch();
        if (debug1)
          Serial.print("crouch");
        break;

      case 9:  //sit
        set_sit();
        if (debug1)
          Serial.print("sit");
        break;

      case 10:  //settle back to level
        move_loops = 1;
        move_steps = 0;
        move_x_axis = 1;
        if (debug1)
          Serial.print("move x 1");
        break;

      case 11:  //march in place
        set_home();
        y_dir = 0;
        x_dir = 0;
        move_loops = 16;
        move_march = 1;
        if (debug1)
          Serial.print("move march");
        break;

      case 12:  //kneel
        set_home();
        set_kneel();
        if (debug1)
          Serial.print("move kneel");
        break;

      case 13:  //y-axis wiggle, short
        spd = 1;
        set_speed();
        move_loops = 10;
        move_steps = 15;
        move_y_axis = 1;
        if (debug1)
          Serial.print("move y short");
        break;

      case 14:  //look left
        spd = 1;
        set_speed();
        move_loops = 1;
        move_steps = 30;
        move_look_left = 1;
        if (debug1)
          Serial.print("move look_left");
        break;

      case 15:  //look right
        spd = 1;
        set_speed();
        move_loops = 1;
        move_steps = 30;
        move_look_right = 1;
        if (debug1)
          Serial.print("move look_right");
        break;

      case 16:  //stand still
        set_stay();
        if (debug1)
          Serial.print("stay");
        break;
    }

    moveDelayInterval = move_delays[i];
    if (debug1) {
      Serial.print("\ti: ");Serial.print(i);Serial.print("\tmove int: ");Serial.println(moveDelayInterval);
    }
    move_delay_sequences[i] = 0;
    break;      //one step per call - the rest wait for the next interval
  }

  if (!moved) {
    //nothing left to run: end the routine and let the eyes droop closed
    move_demo = 0;
    moveDelayInterval = 0;

    use_ramp = 0;
    spd = 5;
    set_speed();

    set_sweep(RFC, servoSpeed[RFC], servoHome[RFC], servoLimit[RFC][0], 7);
    set_sweep(LFC, servoSpeed[LFC], servoHome[LFC], servoLimit[LFC][0], 7);
    set_sweep(RFT, servoSpeed[RFT], servoHome[RFT], servoLimit[RFT][0], 3);
    set_sweep(LFT, servoSpeed[LFT], servoHome[LFT], servoLimit[LFT][0], 3);

    if (debug1) {
      Serial.println(F("\treset DS"));
    }
  }
}


void update_sequencer(int leg, int servo, int sp, float tar, int seq, int del) {
  if (debug3) {
    if (tar) {
      Serial.print("leg: "); Serial.print(leg);
      Serial.print("\tservo: "); Serial.print(servo);
      Serial.print("\tdel: "); Serial.print(del);
      Serial.print("\tpos: "); Serial.print(servoPos[servo]);
      Serial.print("\ttar: "); Serial.print(tar);
      Serial.print("\tseq: "); Serial.println(servoSequence[leg]);
    } else {
      Serial.print(leg); Serial.println(F("-END"));
    }
  }
  servoSpeed[servo] = limit_speed(sp);
  servoSequence[leg] = seq;
  if (tar) {
    servoDelay[servo][0] = del;
    if (del > 0) servoDelay[servo][1] = 1;
    targetPos[servo] = limit_target(servo, tar, 0);
    activeServo[servo] = 1;
  }

  if (use_ramp) {
    set_ramp(servo, servoSpeed[servo], 0, 0, 0, 0);
  }
}
