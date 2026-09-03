/*
 *   NovaSM3 - a Spot-Mini Micro clone
 *   Version: 6.0
 *   Version Date: 2026-08-19
 *
 *   TARGET BOARD: Teensy 4.0  --  this is the MASTER controller.
 *   Its partner is Nova-SM3_nano-v6.0, the slave that owns the display,
 *   the RGB eyes and the ultrasonic sensors.
 *
 *   Original Author:  Chris Locke - cguweb@gmail.com
 *   Nova's website:  https://novaspotmicro.com
 *   GitHub Project:  https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3
 *   YouTube Playlist:  https://www.youtube.com/watch?v=00PkTcGWPvo&list=PLcOZNHwM_I2a3YZKf8FtUjJneKGXCfduk
 *
 *   Version 6.0 is a readability pass over version 5.1 by Jeremy, with
 *   Claude Code. Behaviour is unchanged apart from one documented bug fix
 *   (see the BUGFIX note in ps2_triggers). See README.md for the full list.
 *   
 *   BIG NOTE FROM JEREMY: TECHNICALLY FIRMWARE IS a Frankenstein not based strictly on 5.1....
 *   OLD Versions use ps2 remote control code, new ones use nRF Module and custom remote control
 *   I was too lazy to build a whole remote from scratch, so I used older versions as context for claude code.
 *   Since it uses the ps2 controller instead of nRFModule and a custom remote.
 *
 *   HOW THIS SKETCH IS ORGANISED
 *   ----------------------------
 *   The sketch is split across several tabs. Open them in the Arduino IDE and
 *   they all compile together as one program:
 *
 *     Nova-SM3_teensy-v6.0.ino   globals, setup(), loop()   <- start here
 *     NovaConfig.h               debug flags, fitted hardware, pin map
 *     NovaServos.h               per-robot servo calibration - do not guess at these
 *     NovaSlaveProtocol.h        names for the bytes sent to the Nano
 *     AsyncServo.h               the non-blocking servo motion engine
 *     MPU6050_conf.h             low-level IMU driver
 *     pitches.h                  musical note frequencies
 *     Ps2Remote.ino              reading the remote and turning it into moves
 *     Sensors.ino                PIR, ultrasonic, IMU, current and battery
 *     Poses.ino                  named stances and the servo-state helpers
 *     Motion.ino                 body attitude: roll, pitch, yaw, axis wiggles
 *     Gaits.ino                  the walking gaits
 *     Sequences.ino              scripted multi-step routines and the sequencer
 *     SerialCommands.ino         the typed command interface
 *     SlaveComms.ino             i2c conversation with the Nano
 *     Sound.ino                  buzzer tunes and MP3 playback
 *     Util.ino                   small shared helpers
 *
 *
 *   HOW IT RUNS
 *   -----------
 *   Nothing in this sketch blocks once it is running. loop() does three
 *   things, in this order, on every single pass:
 *
 *     1. update_servos()  - advance every servo one pwm tick towards its
 *                           target, if its own speed timer says it is due
 *     2. one movement     - a chain of move_* flags picks at most ONE
 *                           movement routine to advance this pass
 *     3. poll subsystems  - remote, serial, sensors, each on its own timer
 *
 *   Movement routines never loop or delay. They set targetPos[] and
 *   servoSpeed[] and return; the motion happens later inside AsyncServo.
 *   That is why a routine that "does nothing" is usually a movement flag
 *   that was never set, or one that something else already cleared.
 *
 *
 *   BEFORE YOU RUN IT
 *   -----------------
 *   Set every debug flag in NovaConfig.h to 0 before running Nova untethered.
 *   On a Teensy, printing to a serial port with nothing attached will stall
 *   the board and Nova will appear dead.
 *
 *
 *   KNOWN ISSUES carried over from 5.1
 *   ----------------------------------
 *      the IMU is not as smooth on the Teensy as it was on the Mega
 *      ramping: interrupting a ramp leaves the servo at the interrupted speed
 *      PIR sensitivity is too high at close range (12-24 inches)
 *      integrating IMU data into the movements is still unfinished
 *      the MP3 volume potentiometer on the switch panel is not wired up
 *
 *   Comments marked DEV NOTE mark unfinished work carried over from 5.1.
*/

//build configuration: debug flags, fitted hardware, and the pin map.
//This is the file to edit when changing what is enabled or how it is wired.
#include "NovaConfig.h"

//include supporting libraries
#include <Wire.h>
#include <PS2X_lib.h>
#include <Adafruit_PWMServoDriver.h>
#include <T4_PowerButton.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

//named constants for every byte sent to the Nano slave over i2c.
//included early so the RGB_* / OLED_* macros are defined before first use.
#include "NovaSlaveProtocol.h"

//one row of the serial help screen. Declared up here because the Arduino
//builder generates function prototypes near the top of the sketch, so any
//type used in a function signature must already be visible at this point.
struct HelpRow { const char* input; const char* what; };

/*
   =======================================================
   GLOBAL STATE

   Everything below is runtime state, grouped by the subsystem that owns it.
   Configuration - what is fitted, what is wired where, how much is printed -
   lives in NovaConfig.h instead.

   Two naming conventions run through all of it:
     lastXxxUpdate / xxxInterval   a millis() timer pair polled from loop()
     move_xxx                      a flag that selects a movement routine
   =======================================================
*/

/*
   -------------------------------------------------------
   Servo driver and speed
    :SPEED IS A DELAY. servoSpeed is the number of milliseconds between one
    :pwm tick and the next, so a BIGGER number means a SLOWER servo. That is
    :why min_spd (96) is numerically larger than max_spd (1).
   -------------------------------------------------------
*/
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
byte pwm_oe = 0;                  //1 once servo output has been enabled - see ps2_check()

const float min_spd = 96.0;       //slowest allowed: the longest gap between ticks
const float max_spd = 1.0;        //fastest allowed: the shortest gap between ticks
float default_spd = 13.0;         //speed restored when a routine finishes
float spd = default_spd;          //current global speed
float spd_prev = default_spd;     //saved across a routine that changes speed temporarily

//spd_factor scales the per-servo speeds inside the movement routines, so that
//raising the global speed speeds up a whole gait proportionally. It is
//recalculated from spd by set_speed().
float spd_factor = 1.0;
const float min_spd_factor = 5.0;
const float max_spd_factor = 0.5;

//per-joint speeds used by the generic sequencer, set by whichever routine is running
float spd_c;                      //coxa
float spd_f;                      //femur
float spd_t;                      //tibia

//steering input, in the -22..22 range the sticks are mapped into
float x_dir = 0;                  //+right / -left
float y_dir = 0;                  //+forward / -backward
float z_dir = 0;                  //+up / -down (ride height)

//acceleration / deceleration, applied by set_ramp()
byte use_ramp = 0;                //nothing ramps unless this is set
float ramp_dist = 0.20;           //fraction of the travel spent ramping at each end
                                  //(0.20 = accelerate over the first 20%, decelerate over the last 20%)
float ramp_spd = 5.00;            //how much slower than cruising speed a ramp starts


/*
   -------------------------------------------------------
   DFPlayer Mini MP3 module
    :tracks are played by number, so the SD card contents matter:
    :  1.saber   2.r2one  3.r2two   4.siren  5.chewy    6.radar   7.mariobro
    :  8.laugh   9.what   10.nova   11.hello 12.mode1   13.mode2  14.mode3
    :  15.mode4  16.fon   17.foff   18.sustain 19.warn  20.danger 21.critical
    :  22.halt
    :sounds_req below is that count - if the card holds fewer, boot says so.
   -------------------------------------------------------
*/
static const uint8_t PIN_MP3_TX = 1;
static const uint8_t PIN_MP3_RX = 0;
SoftwareSerial softwareSerial(PIN_MP3_RX, PIN_MP3_TX);
DFRobotDFPlayerMini DFPlayer;

unsigned int mp3Interval = 50;
unsigned long lastMP3Update = 0;
int mp3_queue[5] = {0,0,0,0,0};   //tracks waiting to play, so nothing blocks
int mp3_status = 0;               //1 while the queue is being worked through
int sound_vol = 12;               //current volume (1-30)
int sounds_req = 22;              //how many tracks the sketch expects
int sounds_sd = 0;                //how many the card actually holds
int sound_cnt = 0;                //countdown before follow mode speaks again
int sound_cur = 0;                //reload value for sound_cnt

//volume potentiometer polling. Left in place but currently unread - the pot
//on the switch panel is not wired up. See the DEV note in mp3_volume().
unsigned int potInterval = 10000;
unsigned long lastPotUpdate = 0;


/*
   -------------------------------------------------------
   PIR motion sensors and follow mode
   -------------------------------------------------------
*/
unsigned int pirInterval = 150;
unsigned long lastPIRUpdate = 0;
unsigned int pirDelay = 3;        //cycles of the state machine, NOT milliseconds
int pir_frontState = LOW;
int pir_leftState = LOW;
int pir_rightState = LOW;
int pir_halt = 1;                 //1 = motion triggers the intruder alert
byte pir_reset = 0;               //1 while recovering from an alert
int pir_wait = 0;                 //cycles to ignore the sensors for, after a trigger
byte pir_state = LOW;             //whether an alert is currently running
byte pir_val = 0;
int pir_repeat_cnt = 0;           //consecutive quiet polls, used to stop following
int pir_is_active = pir_active;   //the user's setting, so routines can restore it
int follow_dir = 0;               //which way follow mode last decided to go
int follow_dir_prev = 0;


/*
   -------------------------------------------------------
   Ultrasonic distance sensors (left and right, read via the slave)
   -------------------------------------------------------
*/
unsigned int ussInterval = 250;
unsigned long lastUSSUpdate = 0;
int distance_alarm = 10;              //cm at which an obstacle counts as too close
int distance_alarm_set = 0;           //consecutive triggers, to reject one-off bad reads
int distance_tolerance = 5;           //cm a reading must change by before it is believed
int distance_l;                       //latest left distance
int prev_distance_l;                  //previous left distance, to reject false positives
int distance_r;                       //latest right distance
int prev_distance_r;                  //previous right distance
int uss_is_active = uss_active;       //the user's setting, so routines can restore it


/*
   -------------------------------------------------------
   MPU6050 IMU
    :the register-level driver is in MPU6050_conf.h; these are the fused
    :results and the state get_mpu() keeps between reads
   -------------------------------------------------------
*/
const int MPU = 0x68;
unsigned int mpuInterval = 10;
unsigned int mpuInterval_prev = 400;  //slower interval used for the very first read
unsigned long lastMPUUpdate = 0;
float mroll, mpitch, myaw;            //current fused attitude, in degrees
float mroll_prev, mpitch_prev;        //previous values, for the trigger threshold
float accAngleX, accAngleY, gyroAngleX, gyroAngleY;
float mpu_mroll = 0.00;               //attitude captured at startup, treated as level
float mpu_mpitch = 0.00;
float mpu_myaw = 0.00;
float mpu_trigger_thresh = 0.05;      //degrees of change before the legs respond
float elapsedTime, currentTime, previousTime;
int mpu_is_active = mpu_active;       //the user's setting, so routines can restore it


/*
   -------------------------------------------------------
   PS2 remote
   -------------------------------------------------------
*/
PS2X ps2x;
unsigned int ps2Interval = 50;
unsigned long lastPS2Update = 0;
int ps2_select = 1;               //active button set, 1-4, cycled by SELECT

//ps2 debug reporting, toggled at runtime by the 'ps2' serial command.
//NOTE: set this to 1 here (before flashing) to poll the controller with servo
//      output left DISABLED - lets the remote be checked on the bench without
//      the robot standing up. Toggling it via serial does not touch pwm_oe.
byte ps2_debug = 0;
unsigned int ps2DebugInterval = 120;    //throttles analog stick reporting
unsigned long lastPS2Debug = 0;
unsigned long lastPS2Beat = 0;
const byte ps2_deadzone = 8;            //sticks rarely rest at exactly 128
const unsigned int ps2_btn_mask[16] = {
  PSB_START, PSB_SELECT, PSB_PAD_UP, PSB_PAD_DOWN, PSB_PAD_LEFT, PSB_PAD_RIGHT,
  PSB_TRIANGLE, PSB_CROSS, PSB_CIRCLE, PSB_SQUARE,
  PSB_L1, PSB_R1, PSB_L2, PSB_R2, PSB_L3, PSB_R3
};
const char* const ps2_btn_name[16] = {
  "START", "SELECT", "UP", "DOWN", "LEFT", "RIGHT",
  "TRIANGLE", "CROSS", "CIRCLE", "SQUARE",
  "L1", "R1", "L2", "R2", "L3", "R3"
};


/*
   -------------------------------------------------------
   Slave board and serial commands
   -------------------------------------------------------
*/
byte serial_oled = 0;             //mirrors the slave's mode flag: 0 = system, 1 = OLED.
                                  //Managed by rgb_request() / oled_request(), not by hand.
int serial_resp;                  //last value the slave sent back
unsigned int serialInterval = 60;
unsigned long lastSerialUpdate = 0;
String serial_input;              //characters collected so far for the current command


/*
   -------------------------------------------------------
   Current draw monitoring (ACS712)
    :a servo that has reached its target but is still pulling hard is jammed,
    :which is what amperage_check() is looking for
   -------------------------------------------------------
*/
unsigned int ampInterval = 15000; //shortened automatically as readings get worse
unsigned long lastAmpUpdate = 0;
int amp_cnt = 0;                  //consecutive over-limit readings so far
int amp_thresh = 10;              //how many in a row before shutting the servos down
int amp_warning = 0;              //current warning level, 0-2
int amp_loop = 1;                 //0 stops rescheduling the check
float amp_limit = 6.5;            //amps drawn before the alarm starts counting


/*
   -------------------------------------------------------
   Battery monitoring
    :batt_levels is a descending ladder - each rung crossed raises the warning
    :level shown on the OLED, and the last one halts the system
   -------------------------------------------------------
*/
unsigned long batteryInterval = 3000;
unsigned long lastBatteryUpdate = 0;
int batt_cnt = 0;                 //readings averaged so far
float batt_voltage = 11.8;        //approx fully charged battery minimum nominal voltage
float batt_voltage_prev = 11.8;   //comparison voltage to prevent false positives
float avg_volts = 0;              //running total being averaged
float batt_levels[10] = {         //voltage drop alarm levels(10)
   11.2, 11.1, 11.0, 10.9, 10.8,
   10.7, 10.6, 10.5, 10.4, 10.3,
};


/*
   -------------------------------------------------------
   Movement parameters
    :the numbers a movement routine reads when loop() next advances it
   -------------------------------------------------------
*/
unsigned long lastMoveDelayUpdate = millis();
unsigned int moveDelayInterval = 0;   //0 = no scripted routine is running
int move_delays[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};          //ms to wait at each step
int move_delay_sequences[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //which routine each step runs
int move_delay = 0;               //delay applied between sequenced servo moves
int move_loops = 0;               //repeats left in the current movement, 0 = run forever
int move_switch = 0;              //sub-state used by the multi-phase routines

float move_steps_min = -100;      //bounds the sticks and serial commands map into
float move_steps_max = 100;
float move_steps = 0;             //the main "how far" input to most routines
float move_steps_x = 0;           //kinematics: sideways body offset
float move_steps_y = 0;           //kinematics: forward/back body offset
float move_steps_yaw_x = 0;       //kinematics: yaw driven by the right stick X
float move_steps_yaw_y = 0;       //kinematics: yaw driven by the right stick Y
float move_steps_yaw = 0;         //kinematics: yaw while L3 is held
float move_steps_kx = 0;          //kinematics: x translation
float move_steps_ky = 0;          //kinematics: y translation

//travel limits each input is mapped into, {min, max}
int move_c_steps[2] = {-50, 50};      //coxa
int move_x_steps[2] = {-30, 30};      //sideways
int move_y_steps[2] = {-180, 130};    //forward / backward
int move_z_steps[2] = {-50, 50};      //ride height
float x_dir_steps[2] = {-22, 22};     //stick range for x_dir
float y_dir_steps[2] = {-22, 22};     //stick range for y_dir
float z_dir_steps[2] = {-40, 40};     //stick range for z_dir


/*
   -------------------------------------------------------
   Movement selection flags
    :loop() walks these in order and advances the FIRST one that is set, so at
    :most one movement routine runs per pass. set_stop_active() clears them
    :all - if you add a flag here, add it there too or it will never stop.
   -------------------------------------------------------
*/
byte moving = 0;                  //1 while a gait is mid-stride
byte move_y_axis = 0;
byte move_x_axis = 0;
byte move_roll = 0;
byte move_roll_body = 0;
byte move_pitch = 0;
byte move_pitch_body = 0;
byte move_trot = 0;
byte move_forward = 0;
byte move_backward = 0;           //never set: step_backward() is unimplemented,
                                  //reverse is done with a negative y_dir instead
byte move_left = 0;
byte move_right = 0;
byte move_march = 0;
byte move_wake = 0;
byte move_sequence = 0;
byte move_demo = 0;
byte move_wman = 0;
byte move_funplay = 0;
byte move_look_left = 0;
byte move_look_right = 0;
byte move_roll_x = 0;
byte move_pitch_y = 0;
byte move_kin_x = 0;
byte move_kin_y = 0;
byte move_yaw_x = 0;
byte move_yaw_y = 0;
byte move_yaw = 0;
byte move_servo = 0;              //debug: sweep one servo
byte move_leg = 0;                //debug: sweep one leg
byte move_follow = 0;
String move_paused = "";          //routine to resume once a one-shot move finishes

//centre of gravity compensation. The battery sits towards the rear, so the
//rear legs are asked to travel slightly further than the front ones.
float step_weight_factor = 1;
float step_height_factor = 1.25;  //reported by the "vars" command but not yet
                                  //used by any routine


/*
   -------------------------------------------------------
   Debug movement targets
    :which servo / leg the "servo" and "leg" serial test commands exercise.
    :Changed at runtime with s+ / s- and l+ / l-, so these are only defaults.
   -------------------------------------------------------
*/
byte debug_leg = 0;               //leg index 0-3, see servoLeg[] in NovaServos.h
int debug_servo = 2;              //servo index 0-11, see the RFC..LRT names
int debug_loops = 3;              //how many sweeps one test command runs
int debug_loops2 = 3;             //loops remaining in the test currently running
int debug_spd = 10;               //speed used by the leg sweep test


/*
   -------------------------------------------------------
   Function Prototypes
    :required for functions executed from servo class ( I know, I know, poor OOP design calling functions outside of a class ;p )
   -------------------------------------------------------
*/
void set_ramp(int servo, float sp, float r1_spd, float r1_dist, float r2_spd, float r2_dist);
void amperage_check(int aloop);
void powering_down(void);

//setup melodies
#include "pitches.h"
int melody[] = {
  NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, //1
  NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
  NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,

  NOTE_B4, 16,  NOTE_B5, 16,  NOTE_FS5, 16,   NOTE_DS5, 16,  NOTE_B5, 32,  //2
  NOTE_FS5, -16, NOTE_DS5, 8,  NOTE_DS5, 32, NOTE_E5, 32,  NOTE_F5, 32,
  NOTE_F5, 32,  NOTE_FS5, 32,  NOTE_G5, 32,  NOTE_G5, 32, NOTE_GS5, 32,  NOTE_A5, 16, NOTE_B5, 8
};
int tempo = 105;
int notes = sizeof(melody) / sizeof(melody[0]) / 2;
int wholenote = (60000 * 4) / tempo;
int divider = 0, noteDuration = 0;



//include local class / config files
#include "MPU6050_conf.h"
#include "NovaServos.h"           //include motor setup vars and data arrays
#include "AsyncServo.h"           //include motor class


//instantiate servo objects (s_XXX) with driver reference and servo ID
//coax servo objects
AsyncServo s_RFC(&pwm1, RFC);
AsyncServo s_LFC(&pwm1, LFC);
AsyncServo s_RRC(&pwm1, RRC);
AsyncServo s_LRC(&pwm1, LRC);

//femur servo objects
AsyncServo s_RFF(&pwm1, RFF);
AsyncServo s_LFF(&pwm1, LFF);
AsyncServo s_RRF(&pwm1, RRF);
AsyncServo s_LRF(&pwm1, LRF);

//tibia servo objects
AsyncServo s_RFT(&pwm1, RFT);
AsyncServo s_LFT(&pwm1, LFT);
AsyncServo s_RRT(&pwm1, RRT);
AsyncServo s_LRT(&pwm1, LRT);


/*
   -------------------------------------------------------
   Boot Sequence
    :setup() below is deliberately just a list of steps, in order. Each step
    :lives in its own function immediately after it, so the order Nova brings
    :its hardware up is readable at a glance.
    :
    :ORDER MATTERS in two places:
    :  - the PS2 receiver must be initialised before the PWM driver is allowed
    :    to drive outputs (see init_pwm_and_servos for the full explanation)
    :  - the Nano slave is rebooted before the PWM driver starts, so the eyes
    :    and display are alive to report progress
   -------------------------------------------------------
*/
void setup() {
  boot_init_board();
  boot_open_serial();
  init_power_button();
  init_i2c_bus();
  init_mp3();
  boot_play_intro();
  boot_report_active_settings();
  boot_report_mp3();
  init_mpu();
  init_pir();
  init_amp_monitor();
  init_ps2();
  boot_greet();
  init_slave();
  init_pwm_and_servos();
  boot_play_ready();
  boot_finish();
}


//blink the onboard LED so you can see the board came up at all, and seed random()
void boot_init_board() {
  pinMode(LED_PIN, OUTPUT);
  for (int b = 0; b < 3; b++) {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }

  //seed from the noise on floating analog pin A0, otherwise every boot
  //produces the same "random" sequence for the sounds and idle movements
  randomSeed(analogRead(0));
}

//open the USB serial link and print the banner (debug builds only)
void boot_open_serial() {
  if (debug) {
    Serial.begin(19200);

    //allow serial to connect
//teensy doesn't like this when not connected to serial even with debug disabled?!? 
//    while (!Serial) {
//      delay(1);
//    }
    delay(500);
    Serial.println(F("\n=============================================="));
    Serial.print(F("NOVA SM3 v"));
    Serial.println(VERSION);
    Serial.println(F("=============================================="));
  }
}

//register the Teensy power-button handler that calls powering_down()
void init_power_button() {
  //setup power off button
  set_arm_power_button_callback(&powering_down);
  if (debug) {
    if (arm_power_button_pressed()) {
      Serial.println("System Restarted");
    }
  }
}

//bring up Wire1, the second i2c bus used for the MPU and the Nano slave
void init_i2c_bus() {
  //setup 2nd i2c bus
  Wire1.begin();
  Wire1.setSDA(SDA2_PIN);
  Wire1.setSCL(SCL2_PIN);
}

//bring up the DFPlayer Mini and count the tracks it can see on its SD card
void init_mp3() {
  if (mp3_active) {
    softwareSerial.begin(9600);
  
    //setup DFPlayer Mini
    if (DFPlayer.begin(softwareSerial)) {
      pinMode(MP3_VOL_PIN, INPUT);
      DFPlayer.volume(sound_vol);
      DFPlayer.EQ(DFPLAYER_EQ_ROCK);
      sounds_sd = DFPlayer.readFileCounts();  
    } else if (debug) {
      Serial.println("Connecting to DFPlayer Mini failed!");
    }
  }
}

//the "I am awake" noise, whichever sound hardware is fitted
void boot_play_intro() {
  if (melody_active) {
    if (mp3_active) {
      DFPlayer.volume(20);
      mp3_play(10);
      delay(1500);
      DFPlayer.volume(sound_vol);
      if (rgb_active) {
        rgb_request(RGB_SPEED_250 RGB_COUNT_100 RGB_WIPE_PAIRED);
      }
      mp3_play(1);
      delay(3000);
      if (!quick_boot) {
        mp3_play(2);
        if (!debug) {
          delay(5000);
        }
      }
    } else {
      if (!quick_boot) {
        for (int i=0; i<3; i++) {
          play_phrases();
          delay(random(200,800));
        }
      }
    }
  } else if (buzz_active) {
    if (mp3_active) {
      mp3_play(1);
      if (!debug && !quick_boot) {
        delay(3000);
      }
    } else {
      if (!quick_boot) {
        for (int b = 64; b > 0; b--) {
          tone(BUZZ, b * random(1, 48));
          delay(random(20, 50));
          noTone(BUZZ);
          delay(random(5, 15));
        }
        noTone(BUZZ);
        delay(500);
      }
    }
  }
}

//list which subsystems are switched on, pacing the boot so an IDE-triggered
//reset does not re-enter setup() while the previous upload is still settling
void boot_report_active_settings() {
  //printed in boot order. the LED keeps blinking whether or not a given
  //subsystem is enabled, so the pacing delay is the same on every boot.
  struct SubsystemReport { const byte* enabled; const char* label; };

  //first group: one LED toggle and a long hold each
  static const SubsystemReport slow_group[] = {
    { &slave_active,  "  Slave Circuit"        },
    { &pwm_active,    "  PWM Controller"       },
    { &ps2_active,    "  PS2 Remote"           },
    { &serial_active, "  Serial Commands"      },
    { &mpu_active,    "  MPU6050 IMU"          },
    { &rgb_active,    "  RGB LEDs"             },
    { &oled_active,   "  OLED Display"         },
    { &pir_active,    "  PIR Sensors"          },
  };
  //second group: a full short blink each
  static const SubsystemReport fast_group[] = {
    { &uss_active,    "  Ultra-Sonic Sensors"  },
    { &amp_active,    "  ACS712 Current Sensor"},
    { &batt_active,   "  Battery Monitor"      },
    { &buzz_active,   "  Buzzer"               },
    { &melody_active, "  Melody"               },
  };

  if (!debug || quick_boot) {
    //nothing to print to, so just blink out the same boot delay
    if (!quick_boot) {
      for (int b = 0; b < 4; b++) boot_blink(500);
      for (int b = 0; b < 6; b++) boot_blink(250);
    }
    return;
  }

  Serial.println(F("Active Settings:"));

  byte led_on = 0;
  for (byte i = 0; i < (sizeof(slow_group) / sizeof(slow_group[0])); i++) {
    if (*slow_group[i].enabled) Serial.println(slow_group[i].label);
    led_on = !led_on;
    digitalWrite(LED_PIN, led_on);
    delay(500);
  }

  for (byte i = 0; i < (sizeof(fast_group) / sizeof(fast_group[0])); i++) {
    if (*fast_group[i].enabled) Serial.println(fast_group[i].label);
    boot_blink(250);
  }

  //the MP3 player reports how much of its sound set it actually found
  if (mp3_active) {
    Serial.print(F("  MP3 Player: found "));
    Serial.print(sounds_sd);Serial.print(F(" of "));
    Serial.print(sounds_req);Serial.println(F(" sounds required on disk"));
  }
  boot_blink(250);

  if (!splash_active) Serial.println(F("  Skip Splash Screen"));
  boot_blink(250);

  Serial.println(F("\n==============================================\n"));
}

//one full on/off blink of the onboard LED, used to pace the boot report
void boot_blink(unsigned int hold_ms) {
  digitalWrite(LED_PIN, HIGH);
  delay(hold_ms);
  digitalWrite(LED_PIN, LOW);
  delay(hold_ms);
}

//MP3 progress line, kept separate because it needs the disk counts read above
void boot_report_mp3() {
  if (mp3_active && debug && !quick_boot) {
    Serial.print(F("MP3 Player intializing..."));
    delay(500);
    Serial.println(F("\t\t\tOK"));
  }
}

//self-test, calibrate and start the MPU6050 IMU
void init_mpu() {
  //init mpu6050
  if (mpu_active) {
    Wire1.begin();
    uint8_t c = readByte(MPU6050_ADDRESS, WHO_AM_I_MPU6050);  // Read WHO_AM_I register for MPU-6050
    delay(1000); 
  
    if (c == 0x68) {  
      if (debug) Serial.println(F("MPU6050 testing... "));
      MPU6050SelfTest(SelfTest);
      if (debug) {
        delay(300);
        Serial.println(F("  Acceleration Trim:"));
        Serial.print(F("    x-axis : +/- ")); Serial.println(SelfTest[0],1);
        Serial.print(F("    y-axis : +/- ")); Serial.println(SelfTest[1],1);
        Serial.print(F("    z-axis : +/- ")); Serial.println(SelfTest[2],1);
        Serial.println(F("  Gyration Trim:"));
        Serial.print(F("    x-axis : +/- ")); Serial.println(SelfTest[3],1);
        Serial.print(F("    y-axis : +/- ")); Serial.println(SelfTest[4],1);
        Serial.print(F("    z-axis : +/- ")); Serial.println(SelfTest[5],1);
      }
      if(SelfTest[0] < 1.0f && SelfTest[1] < 1.0f && SelfTest[2] < 1.0f && SelfTest[3] < 1.0f && SelfTest[4] < 1.0f && SelfTest[5] < 1.0f) {
        if (debug) {
          Serial.println(F("  PASSED"));  
          delay(1000);
          if (debug) Serial.print(F("MPU6050 IMU intializing... "));
        }
        calibrateMPU6050(gyroBias, accelBias); // Calibrate gyro and accelerometers, load biases in bias registers  
        initMPU6050(); 
        if (debug) Serial.println(F("\t\t\tOK"));
      } else {
        if (debug) {
          Serial.print(F("  Error: Could not connect to MPU6050 on 0x"));
          Serial.println(c, HEX);
        }
      }
    }
  }
}

//configure the three PIR motion sensor inputs
void init_pir() {
  //init pir sensors
  if (pir_active) {
    pinMode(PIR_FRONT, INPUT);
    digitalWrite(PIR_FRONT, HIGH); 
   
    pinMode(PIR_LEFT, INPUT);
    digitalWrite(PIR_LEFT, HIGH); 
  
    pinMode(PIR_RIGHT, INPUT);
    digitalWrite(PIR_RIGHT, HIGH); 
  }
}

//configure the ACS712 current sensor power control pin
void init_amp_monitor() {
  //init amp power control
  if (amp_active) {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
  }
}

//handshake with the PS2 receiver; on failure ps2_active is cleared so loop() stops polling it
void init_ps2() {
  //init ps2 controller
  if (ps2_active) {
    if (debug) Serial.print(F("PS2 Controller intializing..."));
    delay(500);
    if (ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false) == 0) {
      if (debug) Serial.println(F("\t\t\tOK"));
    } else {
      if (debug) Serial.println(F(" Error!"));
      ps2_active = 0;
    }
  }
}

//eyes/display greeting plus the "stand back" warning before the servos energise
void boot_greet() {
  if (rgb_active) {
    rgb_request(RGB_LEFT RGB_WHITE RGB_RIGHT RGB_WHITE RGB_SPEED_50 RGB_COUNT_100 RGB_PATTERN_BLINK);
  }

  if (oled_active) {
    oled_request(OLED_STAND_BACK);
  }
  if (!quick_boot) {
    if (buzz_active || mp3_active) {
      if (mp3_active) {
        mp3_play(4);
        delay(3000);
        if (!splash_active) {
          delay(2000);
        }
      } else {
        for (int b = 3; b > 0; b--) {
          tone(BUZZ, 440);
          delay(150);
          tone(BUZZ, 220);
          delay(250);
          noTone(BUZZ);
          delay(250);
        }
      }
      digitalWrite(LED_PIN, LOW);
    } else {
      for (int b = 3; b > 0; b--) {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
        delay(250);
      }
    }
  }
}

//reboot the Nano so master and slave start from a known state together
void init_slave() {
  //(re)boot slave nano
  if (slave_active) {
    if (splash_active && !quick_boot) {
      command_slave(SLAVE_REBOOT_SPLASH);
    } else {
      command_slave(SLAVE_REBOOT_QUIET);
    }
  }
  
}

//start the PCA9685, drive every servo to its home pose, then disable output
void init_pwm_and_servos() {
  //init pwm controller
  if (pwm_active) {
    if (oled_active) {
      oled_request(OLED_STAND_BACK);
    }

    if (debug) Serial.print(F("PWM Controller intializing..."));
    pwm1.begin();
    pwm1.setOscillatorFrequency(OSCIL_FREQ);
    pwm1.setPWMFreq(SERVO_FREQ);

    delay(500);
    if (!splash_active) {
      delay(3000);
    }

    if (debug) Serial.println(F("\t\t\tOK"));
    if (debug) {
      Serial.print(TOTAL_SERVOS); Serial.print(F(" Servos intializing..."));
    }

    //set default speed factor
    spd_factor = mapfloat(spd, min_spd, max_spd, min_spd_factor, max_spd_factor);

    //initialize servos and populate related data arrays with defaults
    init_home();
    delay(1000);
    if (debug) Serial.println(F("\t\t\tOK"));

    //DEV NOTE:  OE_PIN disabled motor driver below, because otherwise there is some electrical(?) interference to
    //           the PS2 controller / wiring that was causing the remote to fire all buttons at
    //           arduino boot time, obviously causing Nova to go haywire with unexpected random 
    //           movement commands!
    //
    //           After testing, it appeared to be the PWM controller interfering, so I added code
    //           to halt its output until the PS2 was fully initialized, which is found at the end 
    //           of the ps2_check() function.
    //
    //disable PWM output after initializing servos
    if (ps2_active) {
      digitalWrite(OE_PIN, HIGH);
      delay(100);
    }
  }
}

//the "ready" fanfare once the servos are safely parked
void boot_play_ready() {
  if (melody_active) {
    if (mp3_active) {
      if (!quick_boot) {
        for (int s=0;s<15;s++) {
          DFPlayer.volumeDown();
          delay(250);
        }
        DFPlayer.pause();
        delay(50);
        DFPlayer.volume(25);
        delay(50);
        mp3_play(11);
        if (oled_active) {
          oled_request(OLED_BATTERY_GAUGE);
        }
        if (rgb_active) {
          rgb_request(RGB_SPEED_1000 RGB_FADE_BLUE);
        }
        delay(5000);
        if (oled_active) {
          oled_request(OLED_BATTERY_GAUGE);
        }
        if (rgb_active) {
          rgb_request(RGB_SPEED_500 RGB_FADE_PURPLE);
        }
        delay(4000);
      }
      mp3_play(7);
    } else {
      melody1();
      delay(200);
    }
  } else if (buzz_active) {
    if (mp3_active) {
      mp3_play(7);
    } else {
      for (int b = 0; b < 12; b++) {
        tone(BUZZ, (b*100));
        delay(50);
        noTone(BUZZ);
        delay(25);
      }
      noTone(BUZZ);
    }
  }
}

//restore normal volume, print the ready banner and settle the eyes/display
void boot_finish() {
  if (mp3_active) {
    DFPlayer.volume(sound_vol);
  }
  if (rgb_active) {
    rgb_request(RGB_COUNT_5 RGB_SPEED_50 RGB_FADE_PURPLE);
    delay(1000);
  }

  if (!mpu_active) {
    if (debug) {
      Serial.println(F("\nNova SM3... \t\t\t\tReady!"));
      Serial.println(F("=============================================="));
      if (!plotter && serial_active) {
        Serial.println();
        Serial.println(F("Type a command input or 'h' for help:"));
      }
    }
  }

  if (rgb_active) {
    rgb_request(RGB_COUNT_5 RGB_SPEED_5000 RGB_FADE_YELLOW);
  }
  if (oled_active) {
    oled_request(OLED_READY);
  }
  if (!quick_boot) {
    delay(1000);
  }
}



void loop() {
  update_servos();
/*
   -------------------------------------------------------
   Check for Moves
    :check if any scripted, sequenced, or dynamic moves are active
    :and execute accordingly along with any required variables defined
   -------------------------------------------------------
*/
  if (move_sequence) {
    run_sequence();
  } else if (move_x_axis) {
    x_axis();
  } else if (move_y_axis) {
    y_axis();
  } else if (move_pitch_body) {
    pitch_body();
  } else if (move_pitch) {
    pitch(x_dir);
  } else if (move_roll_body) {
    roll_body();
  } else if (move_roll) {
    roll();
  } else if (move_trot) {
    step_trot(x_dir);
  } else if (move_forward) {
    step_forward(x_dir);
  } else if (move_backward) {
    step_backward(x_dir);
  } else if (move_left) {
    ramp_dist = 0.25;
    ramp_spd = 1.5;
    use_ramp = 0;
    step_left_right(1, x_dir, y_dir);
  } else if (move_right) {
    ramp_dist = 0.25;
    ramp_spd = 1.5;
    use_ramp = 0;
    step_left_right(0, x_dir, y_dir);
  } else if (move_march) {
    step_march(x_dir, y_dir, z_dir);
  } else if (move_wake) {
    wake();
  } else if (move_wman) {
    ramp_dist = 0.2;
    ramp_spd = 0.5;
    use_ramp = 1;
    wman();
  } else if (move_funplay) {
    funplay();
  } else if (move_look_left) {
    look_left();
  } else if (move_look_right) {
    look_right();
  } else if (move_roll_x) {
    roll_x();
  } else if (move_pitch_y) {
    pitch_y();
  } else if (move_kin_x) {
    move_kx();
  } else if (move_kin_y) {
    move_ky();
  } else if (move_yaw_x) {
    yaw_x();
  } else if (move_yaw_y) {
    yaw_y();
  } else if (move_yaw) {
    yaw();
  } else if (move_servo) {
    move_debug_servo();
  } else if (move_leg) {
    move_debug_leg();
  }

/*
   -------------------------------------------------------
   Check State Machines
    :check active state machine(s) for execution time by its respective interval
   -------------------------------------------------------
*/
  if (ps2_active) {
    if (millis() - lastPS2Update > ps2Interval) ps2_check();
  }

  if (serial_active) {
    if (millis() - lastSerialUpdate > serialInterval) serial_check();
  }

  if (pir_active) {
    if (millis() - lastPIRUpdate > pirInterval) pir_check();
  }

  if (mpu_active) {
    if(millis() - lastMPUUpdate > mpuInterval) get_mpu();
  }

  if (uss_active) {
    if (millis() - lastUSSUpdate > ussInterval) uss_check();
  }

  if (amp_active) {
    if (ampInterval && millis() - lastAmpUpdate > ampInterval) amperage_check(amp_loop);
  }

  if (batt_active) {
    if(millis() - lastBatteryUpdate > batteryInterval) battery_check();
  }

  if (moveDelayInterval && millis() - lastMoveDelayUpdate > moveDelayInterval) {
    delay_sequences();
  }

  if (mp3_active && mp3_status) {
    if(millis() - lastMP3Update > mp3Interval) check_mp3();
  }
//  if (potInterval && millis() - lastPotUpdate > potInterval) mp3_volume(-1);
}



/*
   -------------------------------------------------------
   Hardware Functions
   -------------------------------------------------------
*/
/*
   -------------------------------------------------------
   Update Servos
    :check if servo(s) need updating
    :this is the core functionality of Nova
   -------------------------------------------------------
*/
void update_servos() {
  //update coxas
  s_RFC.Update();
  s_LFC.Update();
  s_RRC.Update();
  s_LRC.Update();

  //update femurs
  s_RFF.Update();
  s_LFF.Update();
  s_RRF.Update();
  s_LRF.Update();

  //update tibias
  s_RFT.Update();
  s_LFT.Update();
  s_RRT.Update();
  s_LRT.Update();
}











































void powering_down(void) {
  digitalWriteFast(LED_PIN, !digitalReadFast(LED_PIN));
  if (debug) Serial.println ("System Shutting Down... ");

  if (oled_active) {
    oled_request(OLED_HALTED);
  }
  if (mp3_active) {
    mp3_play(22);
    delay(2500);
    mp3_play(5);
  } else if (buzz_active) {
    for (int b = 12; b > 0; b--) {
      tone(BUZZ, random(1, 12) * 100);
      delay(50);
      noTone(BUZZ);
      delay(25);
    }
    noTone(BUZZ);
  }

  set_stop_active();
  init_home();

  if (rgb_active) {
    rgb_request(RGB_SPEED_1000 RGB_FADE_BLUE);
  }
  if (oled_active) {
    oled_request(OLED_HALTED);
  }
  delay(2500);
}
