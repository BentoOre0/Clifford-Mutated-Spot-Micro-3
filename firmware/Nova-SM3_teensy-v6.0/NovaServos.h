/*
 *   NovaSM3 - a Spot-Mini Micro clone
 *   Version: 6.0
 *   Version Date: 2026-08-19
 *
 *   Original Author:  Chris Locke - cguweb@gmail.com
 *   GitHub Project:  https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3
 *   Thingiverse:  https://www.thingiverse.com/thing:4767006
 *   Instructables Project:  https://www.instructables.com/Nova-Spot-Micro-a-Spot-Mini-Clone/
 *   YouTube Playlist:  https://www.youtube.com/watch?v=00PkTcGWPvo&list=PLcOZNHwM_I2a3YZKf8FtUjJneKGXCfduk
 *
 *   -----------------------------------------------------------------------
 *   SERVO CALIBRATION AND MOTION STATE
 *   -----------------------------------------------------------------------
 *
 *   Two very different things live in this file.
 *
 *   1. CALIBRATION - servoHome[] and servoLimit[] near the bottom. These are
 *      physical measurements of one assembled robot, taken on the bench with
 *      the Nova-SM3-calibrate sketch, in raw PCA9685 pwm ticks. They are not
 *      logic and they are not tunable: if a leg sits wrong, re-measure with
 *      the calibrate sketch rather than nudging numbers here. They are only
 *      meaningful at the SERVO_FREQ / OSCIL_FREQ set in NovaConfig.h.
 *
 *   2. MOTION STATE - the arrays at the top, which AsyncServo reads and
 *      writes every pass of loop(). Several are two-dimensional with a
 *      meaning per column, so the columns are named below; index them with
 *      the RAMP_*, SWEEP_* and DELAY_* constants rather than a bare number.
 *   -----------------------------------------------------------------------
*/

//total counts for reference and iterations
#define TOTAL_SERVOS 12
#define TOTAL_LEGS 4

/*
   -------------------------------------------------------
   Servo names
    :index into every per-servo array below.
    :first letter  R/L   right or left
    :second letter F/R   front or rear
    :third letter  C/F/T coxa (hip swing), femur (upper), tibia (lower)
   -------------------------------------------------------
*/
#define RFC 0
#define RFF 1
#define RFT 2
#define LFC 3
#define LFF 4
#define LFT 5

#define RRC 6
#define RRF 7
#define RRT 8
#define LRC 9
#define LRF 10
#define LRT 11

//leg names, index into servoLeg[] and servoSequence[]
#define RF 0
#define LF 1
#define RR 2
#define LR 3


/*
   -------------------------------------------------------
   Column names for the two-dimensional motion arrays
   -------------------------------------------------------
*/

//servoRamp[servo][...] - acceleration / deceleration profile for one move.
//Written by set_ramp(), consumed and counted down by AsyncServo::Update().
#define RAMP_SPEED       0    //speed to settle back to once ramping is done
#define RAMP_DISTANCE    1    //pwm ticks still to travel
#define RAMP_UP_SPEED    2    //speed the move starts at, before accelerating
#define RAMP_UP_DIST     3    //ticks left in the acceleration phase
#define RAMP_UP_INC      4    //speed change applied per tick while accelerating
#define RAMP_DOWN_SPEED  5    //speed the deceleration phase starts at
#define RAMP_DOWN_DIST   6    //ticks left in the deceleration phase
#define RAMP_DOWN_INC    7    //speed change applied per tick while decelerating

//servoSweep[servo][...] - a servo bouncing between two positions.
//Written by set_sweep(), consumed and counted down by AsyncServo::Update().
#define SWEEP_FROM       0    //one end of the travel
#define SWEEP_TO         1    //the other end, and where the servo heads first
#define SWEEP_DIR        2    //0 = heading towards SWEEP_TO, 1 = heading back
#define SWEEP_LOOPS      3    //out-and-back passes remaining
#define SWEEP_DELAY      4    //state-machine cycles to stall before moving
                              //(not ms). No current caller sets this.
//column 5 is unused and kept only so the array shape does not change

//servoDelay[servo][...] - stalls the start of a sequenced move
#define DELAY_TICKS      0    //state-machine cycles to wait (not ms)
#define DELAY_PENDING    1    //set by update_sequencer, currently never read

//servoLeg[leg][...] - see the JOINT_* names beside servoLeg below


/*
   -------------------------------------------------------
   Per-servo motion state
    :all indexed by the servo names above, all sized TOTAL_SERVOS
   -------------------------------------------------------
*/
byte activeSweep[TOTAL_SERVOS];         //1 while this servo is sweeping
float servoSweep[TOTAL_SERVOS][6];      //sweep parameters, see SWEEP_* above
byte servoSwitch[TOTAL_SERVOS];         //which way a sweeping servo last turned

byte activeServo[TOTAL_SERVOS];         //1 while this servo is moving to a target
float servoSpeed[TOTAL_SERVOS];         //ms between pwm ticks. BIGGER IS SLOWER.
float servoPos[TOTAL_SERVOS];           //where the servo is now, in pwm ticks
float targetPos[TOTAL_SERVOS];          //where it is heading, in pwm ticks
int servoStep[TOTAL_SERVOS];            //ticks taken so far in the current move
float servoRamp[TOTAL_SERVOS][8];       //ramp parameters, see RAMP_* above

int servoSequence[TOTAL_LEGS];          //which step of a multi-step move each leg is on
int servoDelay[TOTAL_SERVOS][2];        //start delay, see DELAY_* above


/*
   -------------------------------------------------------
   Wiring
    :which PCA9685 board and channel each servo is plugged into.

    NOTE the channels are not consecutive: 3, 7, 11 and 15 are skipped, so
    the servo index is NOT the channel number. Always go through this table.
   -------------------------------------------------------
*/
int servoSetup[TOTAL_SERVOS][2] = {       //driver ID, channel number
  {1,0},  {1,1},  {1,2},                  //RFC, RFF, RFT
  {1,4},  {1,5},  {1,6},                  //LFC, LFF, LFT
  {1,8},  {1,9},  {1,10},                 //RRC, RRF, RRT
  {1,12}, {1,13}, {1,14},                 //LRC, LRF, LRT
};

//groups servos into legs, so code can say "the tibia of leg 2" without a
//lookup table of its own. Index the second dimension with the JOINT_* names.
#define JOINT_COXA  0
#define JOINT_FEMUR 1
#define JOINT_TIBIA 2
int servoLeg[TOTAL_LEGS][3] = {       //coxa, femur, tibia
  {RFC,RFF,RFT},  {LFC,LFF,LFT},  {RRC,RRF,RRT},  {LRC,LRF,LRT},
};


/*
   -------------------------------------------------------
   CALIBRATION - measured on one physical robot
    :DO NOT tune these by hand. Regenerate them with Nova-SM3-calibrate.
   -------------------------------------------------------
*/

//the standing pose every movement is measured from, in pwm ticks
float servoHome[TOTAL_SERVOS] = {         //home pos
  352, 280, 510,                          //RFC, RFF, RFT
  364, 442, 225,                          //LFC, LFF, LFT
  367, 331, 423,                          //RRC, RRF, RRT
  364, 351, 213,                          //LRC, LRF, LRT
};

//how far each servo may travel before it fouls the frame or over-extends.
//
//IMPORTANT: these pairs are ordered by LEG DIRECTION, not numerically. For
//the left legs the first value is the LARGER number, because those servos
//are mirrored and count the other way. Anything comparing against these has
//to check which way round the pair is first - see limit_target().
float servoLimit[TOTAL_SERVOS][2] = {     //min, max (in leg direction)
  {314, 434}, {185, 515}, {365, 607},     //RFC, RFF, RFT
  {402, 282}, {537, 207}, {370, 128},     //LFC, LFF, LFT
  {329, 449}, {236, 566}, {278, 520},     //RRC, RRF, RRT
  {402, 282}, {446, 116}, {358, 116},     //LRC, LRF, LRT
};

/*
//DEV/TEST VALUES FOR MINIMAL MOVEMENT!
//swap these in to keep every joint within +/-10 ticks of home, so a new
//build can be exercised on the bench without the legs swinging into anything
float servoLimit[TOTAL_SERVOS][2] = {     //min, max
  {348, 368}, {270, 290}, {495, 515},     //RFC, RFF, RFT
  {374, 354}, {452, 432}, {225, 205},     //LFC, LFF, LFT
  {354, 374}, {331, 351}, {420, 440},     //RRC, RRF, RRT
  {374, 354}, {351, 331}, {285, 265},     //LRC, LRF, LRT
};
*/


//scratch space for multi-step moves: up to six target positions per servo,
//filled in by whichever routine is running and consumed by update_sequencer()
int servoStepMoves[TOTAL_SERVOS][6] = {               //step1, step2, etc
  {0,0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0},        //RFC, RFF, RFT
  {0,0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0},        //LFC, LFF, LFT
  {0,0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0},        //RRC, RRF, RRT
  {0,0,0,0,0,0}, {0,0,0,0,0,0}, {0,0,0,0,0,0},        //LRC, LRF, LRT
};


/*
   -------------------------------------------------------
   Frame measurements
    :not used by any code yet - recorded here for the inverse kinematics
    :work described in the project notes
   -------------------------------------------------------
*/
int bodyBone[3] = {       //bone lengths in mm : length, width, height
  180, 120, 200           //(ie: measured on pivots: shoulder-to-shoulder, coax-to-coax, and ground-to-coax)
};

int servoBone[TOTAL_LEGS][3] = {           //bone lengths in mm : tibia, femur, shoulder
  {132, 105, 90},     //RF
  {132, 105, 90},     //LF
  {132, 105, 90},     //RR
  {132, 105, 90},     //LR
};
