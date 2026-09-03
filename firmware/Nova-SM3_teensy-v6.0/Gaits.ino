/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Walking gaits
 *
 *   The routines that actually make Nova walk. Each is a state machine
 *   driven by servoSequence[]: it checks which step a leg pair has reached,
 *   queues the next one, and returns. They are re-entered from loop() every
 *   pass until the movement flag that selected them is cleared.
 *
 *   The step distances and speed factors are hand-tuned to this frame and its
 *   weight distribution. step_weight_factor biases the rear legs, which carry
 *   more of the battery.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   Walking Gait Functions
   -------------------------------------------------------
*/

/*
 * step_match gait
 * 
 * xdir: +right/-left
 * ydir: +forward/-backward
 * zdir: +up/-down
 * 
 */
void step_march(float xdir, float ydir, float zdir) {

//DEVWORK
//  ramp_dist = 0.2;
//  ramp_spd = 0.3;
//  use_ramp = 1;  

  moving = 1;

  //define move factors
  float cmfact = 0.750;
  float fmfact = 0.700;
  float tmfact = 1.300;

  //define move z-factors
  float czmfact = 0;
  float fzmfact = 0.8;
  float tzmfact = 1.525;

  //define speed factors
  float csfact = 3.00;
  float fsfact = 2.50;
  float tsfact = 5.00;
  float sfact_div = 2.0;

  //calculate zdir positions
  float tz = (zdir * tzmfact);
  float fz = (zdir * fzmfact);
  float cz = czmfact;


  //define home positions 
  //DEVNOTE: this is a bit ugly, but necessary to manipulate home/start position based on zdir
  //
  float gaitHome[TOTAL_SERVOS];
  for (int i = 0; i < TOTAL_SERVOS; i++) {
    gaitHome[i] = servoHome[i];
  }

  //apply z factors by direction, not mid-sequence
  if (zdir < -1) {
    gaitHome[RFT] += abs(tz);
    gaitHome[RFF] -= abs(fz);
    gaitHome[RFC] -= abs(cz);
    gaitHome[RRT] += abs(tz);
    gaitHome[RRF] -= abs(fz);
    gaitHome[RRC] -= abs(cz);
    gaitHome[LFT] -= abs(tz);
    gaitHome[LFF] += abs(fz);
    gaitHome[LFC] += abs(cz);
    gaitHome[LRT] -= abs(tz);
    gaitHome[LRF] += abs(fz);
    gaitHome[LRC] += abs(cz);
  } else if (zdir > 1) {
    gaitHome[RFT] -= tz;
    gaitHome[RFF] += fz;
    gaitHome[RFC] += cz;
    gaitHome[RRT] -= tz;
    gaitHome[RRF] += fz;
    gaitHome[RRC] += cz;
    gaitHome[LFT] += tz;
    gaitHome[LFF] -= fz;
    gaitHome[LFC] -= cz;
    gaitHome[LRT] += tz;
    gaitHome[LRF] -= fz;
    gaitHome[LRC] -= cz;
  }
  
  //calculate steps, speeds, direction, and step_weight_factor
  float servoMS[TOTAL_SERVOS][6][2];   //[servo][step1,step2,etc][dist,spd]

  //coaxes
  //set default marching steps/speed
  servoMS[RFC][0][0] = gaitHome[RFC];
  servoMS[RRC][0][0] = gaitHome[RRC];
  servoMS[LFC][0][0] = gaitHome[LFC];
  servoMS[LRC][0][0] = gaitHome[LRC];
  servoMS[RFC][0][1] = servoMS[LFC][0][1] = servoMS[RRC][0][1] = servoMS[LRC][0][1] = (csfact * spd_factor);
  servoMS[RFC][1][1] = servoMS[LFC][1][1] = servoMS[RRC][1][1] = servoMS[LRC][1][1] = ((csfact * spd_factor) / sfact_div);

  //calc movement and apply weight factor by direction / load shift
  if (xdir < -1) { //turn left
    servoMS[RFC][0][0] = (gaitHome[RFC] + (abs(xdir) * cmfact));
    servoMS[RRC][0][0] = (gaitHome[RRC] + ((abs(xdir) * cmfact) + ((abs(xdir) * cmfact) * (step_weight_factor * cmfact)))); //add weight factor
    servoMS[LFC][0][0] = (gaitHome[LFC] + (abs(xdir) * cmfact));
    servoMS[LRC][0][0] = (gaitHome[LRC] + ((abs(xdir) * cmfact) + ((abs(xdir) * cmfact) * (step_weight_factor * cmfact)))); //add weight factor

    servoMS[RFC][0][1] = servoMS[LFC][0][1] = (csfact * spd_factor);
    servoMS[RRC][0][1] = servoMS[LRC][0][1] = ((csfact - (step_weight_factor * csfact)) * spd_factor);
  } else if (xdir > 1) { //turn right
    servoMS[RFC][0][0] = (gaitHome[RFC] - (xdir * cmfact));
    servoMS[RRC][0][0] = (gaitHome[RRC] - ((xdir * cmfact) + ((xdir * cmfact) * (step_weight_factor * cmfact)))); //add weight factor
    servoMS[LFC][0][0] = (gaitHome[LFC] - (xdir * cmfact));
    servoMS[LRC][0][0] = (gaitHome[LRC] - ((xdir * cmfact) + ((xdir * cmfact) * (step_weight_factor * cmfact)))); //add weight factor

    servoMS[RFC][0][1] = servoMS[LFC][0][1] = (csfact * spd_factor);
    servoMS[RRC][0][1] = servoMS[LRC][0][1] = ((csfact * spd_factor) - (csfact * (step_weight_factor-1)));
  }


  //femurs
  //set default marching steps/speed and step_weight_factor
  servoMS[RFF][0][0] = (gaitHome[RFF] - (move_steps * fmfact));
  servoMS[RRF][0][0] = (gaitHome[RRF] - ((move_steps * fmfact) * step_weight_factor));
  servoMS[LFF][0][0] = (gaitHome[LFF] + (move_steps * fmfact));
  servoMS[LRF][0][0] = (gaitHome[LRF] + ((move_steps * fmfact) * step_weight_factor));
  servoMS[RFF][0][1] = servoMS[LFF][0][1] = (fsfact * spd_factor);
  servoMS[RRF][0][1] = servoMS[LRF][0][1] = ((fsfact * spd_factor) - (fsfact * (step_weight_factor - 1)));
  servoMS[RFF][1][1] = servoMS[LFF][1][1] = servoMS[RRF][1][1] = servoMS[LRF][1][1] = ((fsfact * spd_factor) / sfact_div);

  //adjust step_weight_factor on direction
  if (ydir < -1) { //reverse
    servoMS[RFF][0][0] = (gaitHome[RFF] - ((move_steps * fmfact) * step_weight_factor));
    servoMS[RRF][0][0] = (gaitHome[RRF] - (move_steps * fmfact));
    servoMS[LFF][0][0] = (gaitHome[LFF] + ((move_steps * fmfact) * step_weight_factor));
    servoMS[LRF][0][0] = (gaitHome[LRF] + (move_steps * fmfact));
    servoMS[RRF][0][1] = servoMS[LRF][0][1] = (fsfact * spd_factor);
  } else if (ydir > 1) { //forward
    servoMS[RRF][0][0] = (gaitHome[RRF] - (move_steps * fmfact));
    servoMS[LRF][0][0] = (gaitHome[LRF] + (move_steps * fmfact));
    servoMS[RRF][0][1] = servoMS[LRF][0][1] = (fsfact * spd_factor);
  }


  //tibias
  //adjust move factor on direction
  if (ydir < -1) { //reverse
    tmfact = (tmfact - (abs(ydir) * 0.015));
  } else if (ydir > 1) { //forward
    tmfact = (tmfact + (ydir * 0.015));
  }
  //set default marching steps/speed
  servoMS[RFT][0][0] = (gaitHome[RFT] + (move_steps * tmfact));
  servoMS[RRT][0][0] = (gaitHome[RRT] + ((move_steps * tmfact) * step_weight_factor));
  servoMS[LFT][0][0] = (gaitHome[LFT] - (move_steps * tmfact));
  servoMS[LRT][0][0] = (gaitHome[LRT] - ((move_steps * tmfact) * step_weight_factor));
  servoMS[RFT][0][1] = servoMS[RRT][0][1] = servoMS[LFT][0][1] = servoMS[LRT][0][1] = (tsfact * spd_factor);
  servoMS[RFT][1][1] = servoMS[RRT][1][1] = servoMS[LFT][1][1] = servoMS[LRT][1][1] = ((tsfact * spd_factor) / sfact_div);


  //STEP SEQUENCING
  //to start sequencer, step first paired legs upwards (SEQ1)
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    //SEQ1 RF
    update_sequencer(RF, RFT, servoMS[RFT][0][1], servoMS[RFT][0][0], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFF, servoMS[RFF][0][1], servoMS[RFF][0][0], servoSequence[RF], 0);
    update_sequencer(RF, RFC, servoMS[RFC][0][1], servoMS[RFC][0][0], servoSequence[RF], 0);

    //SEQ1 LR
    update_sequencer(LR, LRT, servoMS[LRT][0][1], servoMS[LRT][0][0], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRF, servoMS[LRF][0][1], servoMS[LRF][0][0], servoSequence[LR], 0);
    update_sequencer(LR, LRC, servoMS[LRC][0][1], servoMS[LRC][0][0], servoSequence[LR], 0);
  }

  //when first paired complete, step second paired legs upwards (SEQ1), and first paired legs downwards (SEQ2)
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 1) {
    //SEQ1 RR
    update_sequencer(RR, RRT, servoMS[RRT][0][1], servoMS[RRT][0][0], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRF, servoMS[RRF][0][1], servoMS[RRF][0][0], servoSequence[RR], 0);
    update_sequencer(RR, RRC, servoMS[RRC][0][1], servoMS[RRC][0][0], servoSequence[RR], 0);

    //SEQ1 LF
    update_sequencer(LF, LFT, servoMS[LFT][0][1], servoMS[LFT][0][0], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFF, servoMS[LFF][0][1], servoMS[LFF][0][0], servoSequence[LF], 0);
    update_sequencer(LF, LFC, servoMS[LFC][0][1], servoMS[LFC][0][0], servoSequence[LF], 0);

    //SEQ2 RF
    update_sequencer(RF, RFT, servoMS[RFT][1][1], gaitHome[RFT], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFF, servoMS[RFF][1][1], gaitHome[RFF], servoSequence[RF], 0);
    update_sequencer(RF, RFC, servoMS[RFC][1][1], gaitHome[RFC], servoSequence[RF], 0);

    //SEQ2 LR
    update_sequencer(LR, LRT, servoMS[LRT][1][1], gaitHome[LRT], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRF, servoMS[LRF][1][1], gaitHome[LRF], servoSequence[LR], 0);
    update_sequencer(LR, LRC, servoMS[LRC][1][1], gaitHome[LRC], servoSequence[LR], 0);
  }

  //when second paired complete, step second paired legs downward (SEQ2), and step first paired legs home (SEQ3)
  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 1) {
    //SEQ2 RR
    update_sequencer(RR, RRT, servoMS[RRT][1][1], gaitHome[RRT], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRF, servoMS[RRF][1][1], gaitHome[RRF], servoSequence[RR], 0);
    update_sequencer(RR, RRC, servoMS[RRC][1][1], gaitHome[RRC], servoSequence[RR], 0);

    //SEQ2 LF
    update_sequencer(LF, LFT, servoMS[LFT][1][1], gaitHome[LFT], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFF, servoMS[LFF][1][1], gaitHome[LFF], servoSequence[LF], 0);
    update_sequencer(LF, LFC, servoMS[LFC][1][1], gaitHome[LFC], servoSequence[LF], 0);

    //reset sequencers
    for (int i = 0; i < TOTAL_LEGS; i++) {
      servoSequence[i] = 0;
    }
    moving = 0;
  }

}


void step_trot(int xdir) {
//DEVWORK

//  ramp_dist = 0.3;
//  ramp_spd = 1.3;
//  use_ramp = 1;  

  float sinc0 = .35;
  float sinc1 = 1.15;

  float sinc2 = .35;
  float sinc3 = 1.15;

  //set coaxes by direction
  servoStepMoves[RFC][0] = servoHome[RFC];
  servoStepMoves[LFC][0] = servoHome[LFC];
  servoStepMoves[RFF][0] = servoHome[RFF];
  servoStepMoves[LFF][0] = servoHome[LFF];
  if (xdir < -1) {  //turn left
    xdir = abs(xdir);
    servoStepMoves[RFC][0] = limit_target(RFC, (servoHome[RFC] - xdir), 15);
    servoStepMoves[LFC][0] = limit_target(LFC, (servoHome[LFC] - (xdir * 0.6)), 20);
    servoStepMoves[RFF][0] = limit_target(RFF, (servoHome[RFF] - (xdir * 0.6)), 0);
    servoStepMoves[LFF][0] = limit_target(LFF, (servoHome[LFF] - (xdir * 2)), 0);

    sinc0 = .25;
    sinc1 = 1.05;
//    sinc2 = .05;
//    sinc3 = 1.05;
  } else if (xdir > +1) {  //turn right
    servoStepMoves[RFC][0] = limit_target(RFC, (servoHome[RFC] + (xdir * 0.6)), 20);
    servoStepMoves[LFC][0] = limit_target(LFC, (servoHome[LFC] + xdir), 15);
    servoStepMoves[RFF][0] = limit_target(RFF, (servoHome[RFF] + (xdir * 2)), 0);
    servoStepMoves[LFF][0] = limit_target(LFF, (servoHome[LFF] + (xdir * 0.6)), 0);

    sinc0 = .25;
    sinc1 = 1.05;
//    sinc2 = .05;
//    sinc3 = 1.05;
  }

  //set tibia sweep movements
  if (!activeSweep[RRT]) {
    update_sequencer(LF, LFC, limit_speed((7 * spd_factor)), servoStepMoves[LFC][0], 0, 0);
    update_sequencer(RF, RFC, limit_speed((7 * spd_factor)), servoStepMoves[RFC][0], 0, 0);
    update_sequencer(LF, LFF, limit_speed((7 * spd_factor)), servoStepMoves[LFF][0], 0, 0);
    update_sequencer(RF, RFF, limit_speed((7 * spd_factor)), servoStepMoves[RFF][0], 0, 0);
    
    set_sweep(LFT, limit_speed((7 * spd_factor)),
              limit_target(LFT, (servoHome[LFT] - (move_steps * sinc0)), 0),
              limit_target(LFT, (servoHome[LFT] + (move_steps * sinc1)), 0), 1);

    set_sweep(RFT, limit_speed((7 * spd_factor)),
              limit_target(RFT, (servoHome[RFT] + (move_steps * sinc0)), 0),
              limit_target(RFT, (servoHome[RFT] - (move_steps * sinc1)), 0), 1);

    set_sweep(LRT, limit_speed((7 * spd_factor)),
              limit_target(LRT, (servoHome[LRT] + (move_steps * sinc2)), 0),
              limit_target(LRT, (servoHome[LRT] - (move_steps * sinc3)), 0), 1);

    set_sweep(RRT, limit_speed((7 * spd_factor)),
              limit_target(RRT, (servoHome[RRT] - (move_steps * sinc2)), 0),
              limit_target(RRT, (servoHome[RRT] + (move_steps * sinc3)), 0), 1);

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_trot = 0;
      }
    }

    lastMoveDelayUpdate = millis();  
  }
}


void step_forward(int xdir) {
//  int lturn = 0;
  int rturn = 0;
  int lfsf = 0;
  int ltsf = 0;
  int rfsf = 0;
  int rtsf = 0;
  if (xdir < 0) {  //turn left
//    lturn = 10;
    rfsf = spd_factor * 0.25;
    rtsf = spd_factor * 0.5;
  } else if (xdir > 0) {  //turn right
    rturn = 10;
    lfsf = spd_factor * 0.25;
    ltsf = spd_factor * 0.5;
  }

  int s1c = 0;
  int s1f = 15;
  int s1t = 35;

  int s2c = 0;
  int s2f = 20 + move_steps;
  int s2t = 15 + move_steps;

  int s3c = 0;
  int s3f = 30 + move_steps;
  int s3t = 10 + move_steps;

  int s4c = 0;
  int s4f = 0;
  int s4t = 0;

  //RF & LR
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    update_sequencer(RF, RFC, (0*spd_factor), (servoHome[RFC] + s1c), (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFF, (4*spd_factor), (servoHome[RFF] - s1f), servoSequence[RF], 0);
    update_sequencer(RF, RFT, (3*spd_factor), (servoHome[RFT] + s1t), servoSequence[RF], 0);

    update_sequencer(LR, LRC, (0*spd_factor), (servoHome[LRC] - s1c), (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRF, (4*spd_factor), (servoHome[LRF] + (s1f * step_weight_factor)), servoSequence[LR], 0);
    update_sequencer(LR, LRT, (3*spd_factor), (servoHome[LRT] - (s1t * step_weight_factor)), servoSequence[LR], 0);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 1) {
    update_sequencer(RF, RFC, (0*spd_factor), (servoHome[RFC] - s2c), (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFF, (3*(spd_factor-rfsf)), (servoHome[RFF] + s2f - rturn), servoSequence[RF], 0);
    update_sequencer(RF, RFT, (6*spd_factor), (servoHome[RFT] + s2t), servoSequence[RF], 0);

    update_sequencer(LR, LRC, (0*spd_factor), (servoHome[LRC] + s2c), (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRF, (3*spd_factor), (servoHome[LRF] - (s2f * step_weight_factor)), servoSequence[LR], 0);
    update_sequencer(LR, LRT, (6*spd_factor), (servoHome[LRT] - (s2t * step_weight_factor)), servoSequence[LR], 0);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
    update_sequencer(RF, RFC, (3*spd_factor), (servoHome[RFC] - s3c), (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFF, (3*spd_factor), (servoHome[RFF] + s3f), servoSequence[RF], 0);
    update_sequencer(RF, RFT, (3*spd_factor), (servoHome[RFT] - s3t), servoSequence[RF], 0);

    update_sequencer(LR, LRC, (3*spd_factor), (servoHome[LRC] + s3c), (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRF, (3*spd_factor), (servoHome[LRF] - (s3f * step_weight_factor)), servoSequence[LR], 0);
    update_sequencer(LR, LRT, (3*spd_factor), (servoHome[LRT] + (s3t * step_weight_factor)), servoSequence[LR], 0);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 3) {
    update_sequencer(RF, RFC, (10*spd_factor), (servoHome[RFC] - s4c), 0, 0);
    update_sequencer(RF, RFF, (10*(spd_factor-rtsf)), (servoHome[RFF] + s4f), 0, 0);
    update_sequencer(RF, RFT, (15*(spd_factor-rtsf)), (servoHome[RFT] - s4t), 0, 0);

    update_sequencer(LR, LRC, (10*spd_factor), (servoHome[LRC] + s4c), 0, 0);
    update_sequencer(LR, LRF, (10*(spd_factor-lfsf)), (servoHome[LRF] - (s4f * step_weight_factor)), 0, 0);
    update_sequencer(LR, LRT, (15*(spd_factor-ltsf)), (servoHome[LRT] + (s4t * step_weight_factor)), 0, 0);
  }

  //LF & RR
  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && !servoSequence[LF] && servoSequence[LR] == 3) {
    update_sequencer(RR, RRC, (0*spd_factor), (servoHome[RRC] + s1c), (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRF, (4*spd_factor), (servoHome[RRF] - (s1f * step_weight_factor)), servoSequence[RR], 0);
    update_sequencer(RR, RRT, (3*spd_factor), (servoHome[RRT] + (s1t * step_weight_factor)), servoSequence[RR], 0);

    update_sequencer(LF, LFC, (0*spd_factor), (servoHome[LFC] - s1c), (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFF, (4*spd_factor), (servoHome[LFF] + s1f), servoSequence[LF], 0);
    update_sequencer(LF, LFT, (3*spd_factor), (servoHome[LFT] - s1t), servoSequence[LF], 0);
  }
  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 1) {
    update_sequencer(RR, RRC, (0*spd_factor), (servoHome[RRC] - s2c), (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRF, (3*(spd_factor-rfsf)), (servoHome[RRF] + (s2f * step_weight_factor) - rturn), servoSequence[RR], 0);
    update_sequencer(RR, RRT, (6*spd_factor), (servoHome[RRT] + (s2t * step_weight_factor)), servoSequence[RR], 0);

    update_sequencer(LF, LFC, (0*spd_factor), (servoHome[LFC] + s2c), (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFF, (3*spd_factor), (servoHome[LFF] - s2f), servoSequence[LF], 0);
    update_sequencer(LF, LFT, (6*spd_factor), (servoHome[LFT] - s2t), servoSequence[LF], 0);
  }
  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 2) {
    update_sequencer(RR, RRC, (3*spd_factor), (servoHome[RRC] - s3c), (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRF, (3*spd_factor), (servoHome[RRF] + (s3f * step_weight_factor)), servoSequence[RR], 0);
    update_sequencer(RR, RRT, (3*spd_factor), (servoHome[RRT] - (s3t * step_weight_factor)), servoSequence[RR], 0);

    update_sequencer(LF, LFC, (3*spd_factor), (servoHome[LFC] + s3c), (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFF, (3*spd_factor), (servoHome[LFF] - s3f), servoSequence[LF], 0);
    update_sequencer(LF, LFT, (3*spd_factor), (servoHome[LFT] + s3t), servoSequence[LF], 0);
  }
  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 3) {
    update_sequencer(RR, RRC, (10*spd_factor), (servoHome[RRC] - s4c), 0, 0);
    update_sequencer(RR, RRF, (10*spd_factor), (servoHome[RRF] + (s4f * step_weight_factor)), 0, 0);
    update_sequencer(RR, RRT, (15*spd_factor), (servoHome[RRT] - (s4t * step_weight_factor)), 0, 0);

    update_sequencer(LF, LFC, (10*spd_factor), (servoHome[LFC] + s4c), 0, 0);
    update_sequencer(LF, LFF, (10*spd_factor), (servoHome[LFF] - s4f), 0, 0);
    update_sequencer(LF, LFT, (15*spd_factor), (servoHome[LFT] + s4t), 0, 0);

    lastMoveDelayUpdate = millis();  
  }

}


void step_backward(int xdir) {
//this is probably not needed, since backwards is just opposite of forwards
}


void step_left_right(int lorr, int xdir, int ydir) {   //where x is +right/-left, and y is +forward/-backward
  spd = 12;  //1-10 (scale with move steps)
  move_steps = 30; //20-110

//scale either move_steps or xdir from the other (leg should lift more when greater xdir, and vice-versa)
//should we push down a bit as first "step" before lifting up?

  if (xdir < 0) xdir = 0;
  if (move_steps < 20) move_steps = 20;

  int rspd_c = limit_speed(spd);
  int rspd_f = limit_speed(spd);
  int rspd_t = limit_speed(spd * 0.5);

  int lspd_c = limit_speed(spd);
  int lspd_f = limit_speed(spd);
  int lspd_t = limit_speed(spd * 0.5);

  servoStepMoves[RFF][0] = limit_target(RFF, (servoHome[RFF] - (0.5 * move_steps)), 10);
  servoStepMoves[RFT][0] = limit_target(RFT, (servoHome[RFT] + (1 * move_steps)), (0.5 * 10));
  servoStepMoves[RRF][0] = limit_target(RRF, (servoHome[RRF] - (0.5 * move_steps)), 10);
  servoStepMoves[RRT][0] = limit_target(RRT, (servoHome[RRT] + (1 * move_steps)), (0.5 * 10));
  servoStepMoves[RFF][2] = servoHome[RFF];
  servoStepMoves[RFT][2] = servoHome[RFT];
  servoStepMoves[RRF][2] = servoHome[RRF];
  servoStepMoves[RRT][2] = servoHome[RRT];

  servoStepMoves[LFF][0] = limit_target(LFF, (servoHome[LFF] + (0.5 * move_steps)), 10);
  servoStepMoves[LFT][0] = limit_target(LFT, (servoHome[LFT] - (1 * move_steps)), (0.5 * 10));
  servoStepMoves[LRF][0] = limit_target(LRF, (servoHome[LRF] + (0.5 * move_steps)), 10);
  servoStepMoves[LRT][0] = limit_target(LRT, (servoHome[LRT] - (1 * move_steps)), (0.5 * 10));
  servoStepMoves[LFF][2] = servoHome[LFF];
  servoStepMoves[LFT][2] = servoHome[LFT];
  servoStepMoves[LRF][2] = servoHome[LRF];
  servoStepMoves[LRT][2] = servoHome[LRT];

  if (lorr == 1) {  //step left
    if (xdir > 1) {
      lspd_f = limit_speed(spd * 1.5);
      lspd_t = limit_speed(spd * 0.75);
      servoStepMoves[LFF][0] = limit_target(LFF, (servoStepMoves[LFF][0] + (0.25 * move_steps)), 10);
      servoStepMoves[LFT][0] = limit_target(LFT, (servoStepMoves[LFT][0] - (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[LRF][0] = limit_target(LRF, (servoStepMoves[LRF][0] + (0.25 * move_steps)), 10);
      servoStepMoves[LRT][0] = limit_target(LRT, (servoStepMoves[LRT][0] - (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[LFF][2] = servoHome[LFF] + (0.125 * move_steps);
      servoStepMoves[LFT][2] = servoHome[LFT] - (0.3625 * move_steps);
      servoStepMoves[LRF][2] = servoHome[LRF] + (0.125 * move_steps);
      servoStepMoves[LRT][2] = servoHome[LRT] - (0.3625 * move_steps);

      servoStepMoves[RFF][0] = limit_target(RFF, (servoStepMoves[RFF][0] + (0.25 * move_steps)), 10);
      servoStepMoves[RFT][0] = limit_target(RFT, (servoStepMoves[RFT][0] - (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[RRF][0] = limit_target(RRF, (servoStepMoves[RRF][0] + (0.25 * move_steps)), 10);
      servoStepMoves[RRT][0] = limit_target(RRT, (servoStepMoves[RRT][0] - (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[RFF][2] = servoHome[RFF] + (0.5 * move_steps);
      servoStepMoves[RFT][2] = servoHome[RFT] - (move_steps);
      servoStepMoves[RRF][2] = servoHome[RRF] + (0.5 * move_steps);
      servoStepMoves[RRT][2] = servoHome[RRT] - (move_steps);
    }
  } else {  //step right
    if (xdir > 1) {
      rspd_f = limit_speed(spd * 1.5);
      rspd_t = limit_speed(spd * 0.75);
      servoStepMoves[RFF][0] = limit_target(RFF, (servoStepMoves[RFF][0] - (0.25 * move_steps)), 10);
      servoStepMoves[RFT][0] = limit_target(RFT, (servoStepMoves[RFT][0] + (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[RRF][0] = limit_target(RRF, (servoStepMoves[RRF][0] - (0.25 * move_steps)), 10);
      servoStepMoves[RRT][0] = limit_target(RRT, (servoStepMoves[RRT][0] + (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[RFF][2] = servoHome[RFF] - (0.125 * move_steps);
      servoStepMoves[RFT][2] = servoHome[RFT] + (0.3625 * move_steps);
      servoStepMoves[RRF][2] = servoHome[RRF] - (0.125 * move_steps);
      servoStepMoves[RRT][2] = servoHome[RRT] + (0.3625 * move_steps);

      servoStepMoves[LFF][0] = limit_target(LFF, (servoStepMoves[LFF][0] - (0.25 * move_steps)), 10);
      servoStepMoves[LFT][0] = limit_target(LFT, (servoStepMoves[LFT][0] + (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[LRF][0] = limit_target(LRF, (servoStepMoves[LRF][0] - (0.25 * move_steps)), 10);
      servoStepMoves[LRT][0] = limit_target(LRT, (servoStepMoves[LRT][0] + (0.5 * move_steps)), (0.5 * 10));
      servoStepMoves[LFF][2] = servoHome[LFF] - (0.5 * move_steps);
      servoStepMoves[LFT][2] = servoHome[LFT] + (move_steps);
      servoStepMoves[LRF][2] = servoHome[LRF] - (0.5 * move_steps);
      servoStepMoves[LRT][2] = servoHome[LRT] + (move_steps);
    }
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    update_sequencer(RF, RFC, rspd_c, servoHome[RFC], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, rspd_t, servoStepMoves[RFT][0], servoSequence[RF], 0);
    update_sequencer(RF, RFF, rspd_f, servoStepMoves[RFF][0], servoSequence[RF], 0);
    update_sequencer(RR, RRC, rspd_c, servoHome[RRC], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, rspd_t, servoStepMoves[RRT][0], servoSequence[RR], 0);
    update_sequencer(RR, RRF, rspd_f, servoStepMoves[RRF][0], servoSequence[RR], 0);
  }

  if (!activeServo[RFC] && servoSequence[RF] == 1) {
    if (lorr == 1) {  //step left
      if (xdir < 1) {
        servoStepMoves[RFC][1] = servoHome[RFC];
        servoStepMoves[RRC][1] = servoHome[RRC];
      } else {
        servoStepMoves[RFC][1] = (servoHome[RFC] + xdir);
        servoStepMoves[RRC][1] = (servoHome[RRC] + xdir);
      }
    } else {  //step right
      if (!xdir) {
        servoStepMoves[RFC][1] = servoHome[RFC];
        servoStepMoves[RRC][1] = servoHome[RRC];
      } else {
        servoStepMoves[RFC][1] = (servoHome[RFC] - xdir);
        servoStepMoves[RRC][1] = (servoHome[RRC] - xdir);
      }
    }
    update_sequencer(RF, RFC, (rspd_c), servoStepMoves[RFC][1], (servoSequence[RF] + 1), 0);
    update_sequencer(RR, RRC, (rspd_c), servoStepMoves[RRC][1], (servoSequence[RR] + 1), 0);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
    update_sequencer(RF, RFC, (rspd_c/3), servoPos[RFC], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, (rspd_t/3), servoHome[RFT], servoSequence[RF], 0);
    update_sequencer(RF, RFF, (rspd_f/3), servoHome[RFF], servoSequence[RF], 0);
    update_sequencer(RR, RRC, (rspd_c/3), servoPos[RRC], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, (rspd_t/3), servoHome[RRT], servoSequence[RR], 0);
    update_sequencer(RR, RRF, (rspd_f/3), servoHome[RRF], servoSequence[RR], 0);
  }

  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 3) {
    //Serial.println("\treset");
    update_sequencer(RF, RFC, rspd_c, servoHome[RFC], 0, 0);
    update_sequencer(RF, RFT, rspd_t, servoHome[RFT], 0, 0);
    update_sequencer(RF, RFF, rspd_f, servoHome[RFF], 0, 0);
    update_sequencer(RR, RRC, rspd_c, servoHome[RRC], 0, 0);
    update_sequencer(RR, RRT, rspd_t, servoHome[RRT], 0, 0);
    update_sequencer(RR, RRF, rspd_f, servoHome[RRF], 0, 0);
  }

  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && !servoSequence[LF] && servoSequence[RF] == 3) {
    update_sequencer(LF, LFC, lspd_c, servoHome[LFC], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, lspd_t, servoStepMoves[LFT][0], servoSequence[LF], 0);
    update_sequencer(LF, LFF, lspd_f, servoStepMoves[LFF][0], servoSequence[LF], 0);
    update_sequencer(LR, LRC, lspd_c, servoHome[LRC], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, lspd_t, servoStepMoves[LRT][0], servoSequence[LR], 0);
    update_sequencer(LR, LRF, lspd_f, servoStepMoves[LRF][0], servoSequence[LR], 0);  
  }

  if (!activeServo[LFC] && servoSequence[LF] == 1) {
    if (lorr == 1) {  //step left
      if (xdir < 1) {
        servoStepMoves[LFC][1] = servoHome[LFC];
        servoStepMoves[LRC][1] = servoHome[LRC];
      } else {
        servoStepMoves[LFC][1] = (servoHome[LFC] + xdir);
        servoStepMoves[LRC][1] = (servoHome[LRC] + xdir);
      }
    } else {  //step right
      if (xdir < 1) {
        servoStepMoves[LFC][1] = servoHome[LFC];
        servoStepMoves[LRC][1] = servoHome[LRC];
      } else {
        servoStepMoves[LFC][1] = (servoHome[LFC] - xdir);
        servoStepMoves[LRC][1] = (servoHome[LRC] - xdir);
      }
    }
    update_sequencer(LF, LFC, (lspd_c), servoStepMoves[LFC][1], (servoSequence[LF] + 1), 0);
    update_sequencer(LR, LRC, (lspd_c), servoStepMoves[LRC][1], (servoSequence[LR] + 1), 0);
  }

  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 2) {
    update_sequencer(LF, LFC, (lspd_c/3), servoPos[LFC], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, (lspd_t/3), servoHome[LFT], servoSequence[LF], 0);
    update_sequencer(LF, LFF, (lspd_f/3), servoHome[LFF], servoSequence[LF], 0);
    update_sequencer(LR, LRC, (lspd_c/3), servoPos[LRC], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, (lspd_t/3), servoHome[LRT], servoSequence[LR], 0);
    update_sequencer(LR, LRF, (lspd_f/3), servoHome[LRF], servoSequence[LR], 0);
  }
  
  if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 3) {
    //Serial.println("\treset");
    update_sequencer(LF, LFC, lspd_c, servoHome[LFC], 0, 0);
    update_sequencer(LF, LFT, lspd_t, servoHome[LFT], 0, 0);
    update_sequencer(LF, LFF, lspd_f, servoHome[LFF], 0, 0);
    update_sequencer(LR, LRC, lspd_c, servoHome[LRC], 0, 0);
    update_sequencer(LR, LRT, lspd_t, servoHome[LRT], 0, 0);
    update_sequencer(LR, LRF, lspd_f, servoHome[LRF], 0, 0);

    lastMoveDelayUpdate = millis();  

    if (move_loops) {
      move_loops--;
      if (!move_loops) {
        move_left = 0;
        move_right = 0;
      }
    }
  
  }
}


void wake() {
  servoStepMoves[RFF][0] = 15;
  servoStepMoves[LRF][0] = 15;
  servoStepMoves[LFF][0] = 15;
  servoStepMoves[RRF][0] = 15;
  servoStepMoves[RFT][0] = 20;
  servoStepMoves[LRT][0] = 20;
  servoStepMoves[LFT][0] = 20;
  servoStepMoves[RRT][0] = 20;

  if (move_loops) {
    //RF
    if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
      if (move_loops) {
        if (!move_switch) {
          if (move_loops == 1) {
            move_loops = 10;
            move_switch = 1;
          }
          servoStepMoves[RFC][0] = 0;
          servoStepMoves[LRC][0] = 0;
          servoStepMoves[LFC][0] = 0;
          servoStepMoves[RRC][0] = 0;
        } else if (move_switch == 2) {
          servoStepMoves[RFC][0] = 0;
          servoStepMoves[LRC][0] = 0;
          servoStepMoves[LFC][0] = 0;
          servoStepMoves[RRC][0] = 0;
        }
      }

      update_sequencer(RF, RFC, 1, (servoPos[RFC] - servoStepMoves[RFC][0]), (servoSequence[RF] + 1), 0);
      update_sequencer(RF, RFF, 1, (servoHome[RFF] - servoStepMoves[RFF][0]), servoSequence[RF], 0);
      update_sequencer(RF, RFT, 1, (servoHome[RFT] + servoStepMoves[RFT][0]), servoSequence[RF], 0);
    }
    if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 1) {
      update_sequencer(RF, RFC, 1, servoPos[RFC], (servoSequence[RF] + 1), 0);
      update_sequencer(RF, RFF, 1, servoHome[RFF], servoSequence[RF], 0);
      update_sequencer(RF, RFT, 1, servoHome[RFT], servoSequence[RF], 0);
    }

    //LR
    if (!activeServo[LRC] && !activeServo[LRF] && !activeServo[LRT] && !servoSequence[LR] && !activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
      update_sequencer(LR, LRC, 1, (servoPos[LRC] + servoStepMoves[LRC][0]), (servoSequence[LR] + 1), 0);
      update_sequencer(LR, LRF, 1, (servoHome[LRF] + servoStepMoves[LRF][0]), servoSequence[LR], 0);
      update_sequencer(LR, LRT, 1, ((servoHome[LRT] - servoStepMoves[LRT][0]) - (servoStepMoves[LRT][0]*step_weight_factor)), servoSequence[LR], 0);
    }
    if (!activeServo[LRC] && !activeServo[LRF] && !activeServo[LRT] && servoSequence[LR] == 1) {
      update_sequencer(LR, LRC, 1, servoPos[LRC], (servoSequence[LR] + 1), 0);
      update_sequencer(LR, LRF, 1, servoHome[LRF], servoSequence[LR], 0);
      update_sequencer(LR, LRT, 1, servoHome[LRT], servoSequence[LR], 0);
    }

    //LF
    if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && !servoSequence[LF] && !activeServo[LRC] && !activeServo[LRF] && !activeServo[LRT] && servoSequence[LR] == 2) {
      update_sequencer(LF, LFC, 1, (servoPos[LFC] + servoStepMoves[LFC][0]), (servoSequence[LF] + 1), 0);
      update_sequencer(LF, LFF, 1, (servoHome[LFF] + servoStepMoves[LFF][0]), servoSequence[LF], 0);
      update_sequencer(LF, LFT, 1, (servoHome[LFT] - servoStepMoves[LFT][0]), servoSequence[LF], 0);
    }
    if (!activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 1) {
      update_sequencer(LF, LFC, 1, servoPos[LFC], (servoSequence[LF] + 1), 0);
      update_sequencer(LF, LFF, 1, servoHome[LFF], servoSequence[LF], 0);
      update_sequencer(LF, LFT, 1, servoHome[LFT], servoSequence[LF], 0);
    }

    //RR
    if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && !servoSequence[RR] && !activeServo[LFC] && !activeServo[LFF] && !activeServo[LFT] && servoSequence[LF] == 2) {
      update_sequencer(RR, RRC, 1, (servoPos[RRC] - servoStepMoves[RRC][0]), (servoSequence[RR] + 1), 0);
      update_sequencer(RR, RRF, 1, (servoHome[RRF] - servoStepMoves[RRF][0]), servoSequence[RR], 0);
      update_sequencer(RR, RRT, 1, ((servoHome[RRT] + servoStepMoves[RRT][0]) + (servoStepMoves[RRT][0]*step_weight_factor)), servoSequence[RR], 0);
    }
    if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 1) {
      update_sequencer(RR, RRC, 1, servoPos[RRC], (servoSequence[RR] + 1), 0);
      update_sequencer(RR, RRF, 1, servoHome[RRF], servoSequence[RR], 0);
      update_sequencer(RR, RRT, 1, servoHome[RRT], servoSequence[RR], 0);


      for (int i = 0; i < TOTAL_LEGS; i++) {
        servoSequence[i] = 0;
      }

      move_loops--;
    }

    lastMoveDelayUpdate = millis();  
  } else if (move_switch == 1) {
    move_loops = 9;
    move_switch = 2;
  } else if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && !servoSequence[RR]) {
    spd = default_spd;
    set_speed();
    move_wake = 0;
  }
}


void wman() {

  servoStepMoves[RFF][0] = servoHome[RFF] - 17;
  servoStepMoves[RFT][0] = servoHome[RFT] + 77;
  servoStepMoves[RFF][1] = servoHome[RFF] - 32;
  servoStepMoves[RFT][1] = servoHome[RFT] + 77;
  servoStepMoves[RFF][2] = servoHome[RFF] + 44;
  servoStepMoves[RFT][2] = servoHome[RFT] - 14;
  servoStepMoves[RFF][3] = servoHome[RFF] - 12;
  servoStepMoves[RFT][3] = servoHome[RFT] - 4;
  servoStepMoves[RFF][4] = servoHome[RFF] - 24;
  servoStepMoves[RFT][4] = servoHome[RFT] + 5;
  servoStepMoves[RFF][5] = servoHome[RFF] - 16;
  servoStepMoves[RFT][5] = servoHome[RFT] - 8;

  servoStepMoves[LRF][0] = servoHome[LRF] + 17;
  servoStepMoves[LRT][0] = servoHome[LRT] - 77;
  servoStepMoves[LRF][1] = servoHome[LRF] + 32;
  servoStepMoves[LRT][1] = servoHome[LRT] - 77;
  servoStepMoves[LRF][2] = servoHome[LRF] - 44;
  servoStepMoves[LRT][2] = servoHome[LRT] + 14;
  servoStepMoves[LRF][3] = servoHome[LRF] + 12;
  servoStepMoves[LRT][3] = servoHome[LRT] + 4;
  servoStepMoves[LRF][4] = servoHome[LRF] + 24;
  servoStepMoves[LRT][4] = servoHome[LRT] - 5;
  servoStepMoves[LRF][5] = servoHome[LRF] + 16;
  servoStepMoves[LRT][5] = servoHome[LRT] + 8;


  //SEQ 1 : RF & LR
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && !servoSequence[RF]) {
    update_sequencer(RF, RFF, 5*spd_factor, servoStepMoves[RFF][0], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, 1*spd_factor, servoStepMoves[RFT][0], servoSequence[RF], 0);

    update_sequencer(LR, LRF, 5*spd_factor, servoStepMoves[LRF][0], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, 1*spd_factor, servoStepMoves[LRT][0], servoSequence[LR], 0);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 1) {
    update_sequencer(RF, RFF, 7, servoStepMoves[RFF][1], (servoSequence[RF] + 1), 5);
    update_sequencer(RF, RFT, 7, servoStepMoves[RFT][1], servoSequence[RF], 5);

    update_sequencer(LR, LRF, 7, servoStepMoves[LRF][1], (servoSequence[LR] + 1), 5);
    update_sequencer(LR, LRT, 7, servoStepMoves[LRT][1], servoSequence[LR], 5);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 2) {
    use_ramp = 0;
    update_sequencer(RF, RFF, 1, servoStepMoves[RFF][2], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, 0.5, servoStepMoves[RFT][2], servoSequence[RF], 0);

    update_sequencer(LR, LRF, 1, servoStepMoves[LRF][2], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, 0.5, servoStepMoves[LRT][2], servoSequence[LR], 0);
    use_ramp = 1;
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 3) {
    update_sequencer(RF, RFF, 14*(0.55*spd_factor), servoStepMoves[RFF][3], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, 44*(0.55*spd_factor), servoStepMoves[RFT][3], servoSequence[RF], 0);

    update_sequencer(LR, LRF, 14*(0.55*spd_factor), servoStepMoves[LRF][3], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, 44*(0.55*spd_factor), servoStepMoves[LRT][3], servoSequence[LR], 0);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 4) {
    update_sequencer(RF, RFF, 2*spd_factor, servoStepMoves[RFF][4], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, 1*spd_factor, servoStepMoves[RFT][4], servoSequence[RF], 0);

    update_sequencer(LR, LRF, 2*spd_factor, servoStepMoves[LRF][4], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, 1*spd_factor, servoStepMoves[LRT][4], servoSequence[LR], 0);
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 5) {
    use_ramp = 0;
    update_sequencer(RF, RFF, 3, servoStepMoves[RFF][5], (servoSequence[RF] + 1), 0);
    update_sequencer(RF, RFT, 1.5, servoStepMoves[RFT][5], servoSequence[RF], 0);

    update_sequencer(LR, LRF, 3, servoStepMoves[LRF][5], (servoSequence[LR] + 1), 0);
    update_sequencer(LR, LRT, 1.5, servoStepMoves[LRT][5], servoSequence[LR], 0);
    use_ramp = 1;
  }
  if (!activeServo[RFC] && !activeServo[RFF] && !activeServo[RFT] && servoSequence[RF] == 6) {
    update_sequencer(RF, RFF, 3*spd_factor, servoHome[RFF], 0, 10);
    update_sequencer(RF, RFT, 3*spd_factor, servoHome[RFT], 0, 10);
    update_sequencer(LR, LRF, 3*spd_factor, servoHome[LRF], 0, 10);
    update_sequencer(LR, LRT, 3*spd_factor, servoHome[LRT], 0, 10);
  }


  servoStepMoves[RRF][0] = servoHome[RRF] - 17;
  servoStepMoves[RRT][0] = servoHome[RRT] + 77;
  servoStepMoves[RRF][1] = servoHome[RRF] - 32;
  servoStepMoves[RRT][1] = servoHome[RRT] + 77;
  servoStepMoves[RRF][2] = servoHome[RRF] + 44;
  servoStepMoves[RRT][2] = servoHome[RRT] - 14;
  servoStepMoves[RRF][3] = servoHome[RRF] - 12;
  servoStepMoves[RRT][3] = servoHome[RRT] - 4;
  servoStepMoves[RRF][4] = servoHome[RRF] - 24;
  servoStepMoves[RRT][4] = servoHome[RRT] + 5;
  servoStepMoves[RRF][5] = servoHome[RRF] - 16;
  servoStepMoves[RRT][5] = servoHome[RRT] - 8;

  servoStepMoves[LFF][0] = servoHome[LFF] + 17;
  servoStepMoves[LFT][0] = servoHome[LFT] - 77;
  servoStepMoves[LFF][1] = servoHome[LFF] + 32;
  servoStepMoves[LFT][1] = servoHome[LFT] - 77;
  servoStepMoves[LFF][2] = servoHome[LFF] - 44;
  servoStepMoves[LFT][2] = servoHome[LFT] + 14;
  servoStepMoves[LFF][3] = servoHome[LFF] + 12;
  servoStepMoves[LFT][3] = servoHome[LFT] + 4;
  servoStepMoves[LFF][4] = servoHome[LFF] + 24;
  servoStepMoves[LFT][4] = servoHome[LFT] - 5;
  servoStepMoves[LFF][5] = servoHome[LFF] + 16;
  servoStepMoves[LFT][5] = servoHome[LFT] + 8;


  //SEQ 1 : RR & LF
  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && !servoSequence[RR] && servoSequence[RF] == 4) {
    update_sequencer(RR, RRF, 5*spd_factor, servoStepMoves[RRF][0], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, 1*spd_factor, servoStepMoves[RRT][0], servoSequence[RR], 0);

    update_sequencer(LF, LFF, 5*spd_factor, servoStepMoves[LFF][0], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, 1*spd_factor, servoStepMoves[LFT][0], servoSequence[LF], 0);
  }
  if (!activeServo[RFC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 1) {
    update_sequencer(RR, RRF, 7, servoStepMoves[RRF][1], (servoSequence[RR] + 1), 5);
    update_sequencer(RR, RRT, 7, servoStepMoves[RRT][1], servoSequence[RR], 5);

    update_sequencer(LF, LFF, 7, servoStepMoves[LFF][1], (servoSequence[LF] + 1), 5);
    update_sequencer(LF, LFT, 7, servoStepMoves[LFT][1], servoSequence[LF], 5);
  }
  if (!activeServo[RFC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 2) {
    use_ramp = 0;
    update_sequencer(RR, RRF, 1, servoStepMoves[RRF][2], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, 0.5, servoStepMoves[RRT][2], servoSequence[RR], 0);

    update_sequencer(LF, LFF, 1, servoStepMoves[LFF][2], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, 0.5, servoStepMoves[LFT][2], servoSequence[LF], 0);
    use_ramp = 1;
  }
  if (!activeServo[RFC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 3) {
    update_sequencer(RR, RRF, 14*(0.55*spd_factor), servoStepMoves[RRF][3], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, 44*(0.55*spd_factor), servoStepMoves[RRT][3], servoSequence[RR], 0);

    update_sequencer(LF, LFF, 14*(0.55*spd_factor), servoStepMoves[LFF][3], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, 44*(0.55*spd_factor), servoStepMoves[LFT][3], servoSequence[LF], 0);
  }
  if (!activeServo[RFC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 4) {
    update_sequencer(RR, RRF, 2*spd_factor, servoStepMoves[RRF][4], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, 1*spd_factor, servoStepMoves[RRT][4], servoSequence[RR], 0);

    update_sequencer(LF, LFF, 2*spd_factor, servoStepMoves[LFF][4], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, 1*spd_factor, servoStepMoves[LFT][4], servoSequence[LF], 0);
  }
  if (!activeServo[RFC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 5) {
    use_ramp = 0;
    update_sequencer(RR, RRF, 3, servoStepMoves[RRF][5], (servoSequence[RR] + 1), 0);
    update_sequencer(RR, RRT, 1.5, servoStepMoves[RRT][5], servoSequence[RR], 0);

    update_sequencer(LF, LFF, 3, servoStepMoves[LFF][5], (servoSequence[LF] + 1), 0);
    update_sequencer(LF, LFT, 1.5, servoStepMoves[LFT][5], servoSequence[LF], 0);
    use_ramp = 1;
  }
  if (!activeServo[RRC] && !activeServo[RRF] && !activeServo[RRT] && servoSequence[RR] == 6) {
    update_sequencer(RR, RRF, 3*spd_factor, servoHome[RRF], 0, 10);
    update_sequencer(RR, RRT, 3*spd_factor, servoHome[RRT], 0, 10);
    update_sequencer(LF, LFF, 3*spd_factor, servoHome[LFF], 0, 10);
    update_sequencer(LF, LFT, 3*spd_factor, servoHome[LFT], 0, 10);

    lastMoveDelayUpdate = millis();  
  }
}
