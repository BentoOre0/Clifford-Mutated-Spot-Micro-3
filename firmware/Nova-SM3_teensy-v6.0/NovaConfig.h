/*
 *   NovaSM3 - a Spot-Mini Micro clone
 *   Version: 6.0
 *   Version Date: 2026-08-19
 *
 *   Original Author:  Chris Locke - cguweb@gmail.com
 *   Nova's website:  https://novaspotmicro.com
 *   GitHub Project:  https://github.com/cguweb-com/Arduino-Projects/tree/main/Nova-SM3
 *
 *   -----------------------------------------------------------------------
 *   BUILD CONFIGURATION - this is the file you edit
 *   -----------------------------------------------------------------------
 *
 *   Everything here describes THIS robot and THIS build: which subsystems are
 *   fitted, how much the sketch prints while it runs, and what is wired where.
 *   Nothing here is state - runtime variables live in the main sketch next to
 *   the code that uses them.
 *
 *   Per-servo calibration is NOT here. Home positions and travel limits are
 *   physical measurements of one assembled robot and live in NovaServos.h.
 *   -----------------------------------------------------------------------
*/

#ifndef NOVA_CONFIG_H
#define NOVA_CONFIG_H

#define VERSION 6.0


/*
   -------------------------------------------------------
   Debug output
    :each flag enables one category of Serial.print tracing

    *** READ THIS BEFORE RUNNING NOVA OFF THE TETHER ***
    On a Teensy, printing to a serial port that nothing is listening on will
    stall the board. If ANY flag below is set, Nova will appear dead until you
    plug in USB. Set them all to 0 before running untethered.

    `plotter` is different: it suppresses the ordinary human-readable prints
    so the remaining numeric output can be fed to the Arduino Serial Plotter,
    which only understands "label:value" lines. That is why so much printing
    in this sketch is wrapped in `if (!plotter)`.
   -------------------------------------------------------
*/
const byte debug   = 1;           //general progress and boot messages
const byte debug1  = 0;           //ps2 button presses and PIR sensor events
const byte debug2  = 0;           //individual servo steps
const byte debug3  = 0;           //ramping and move sequencing
const byte debug4  = 1;           //amperage draw and battery voltage
const byte debug5  = 0;           //MPU6050 roll / pitch / yaw
const byte debug6  = 0;           //i2c traffic to and from the Nano slave
const byte debug7  = 0;           //ultrasonic sensor distances
const byte plotter = 0;           //format output for the Serial Plotter (turn debug1 off)


/*
   -------------------------------------------------------
   Fitted hardware
    :set to 0 for anything this robot does not have, or that you want to keep
    :quiet while testing something else. Each flag is checked both in setup()
    :(to skip initialising the device) and in loop() (to skip polling it).
    :
    :Several are also mirrored into a `*_is_active` variable at startup, so
    :routines can switch a subsystem off temporarily and put it back exactly
    :as the user configured it here - see the main sketch.
   -------------------------------------------------------
*/
byte slave_active   = 1;          //Arduino Nano slave: OLED, RGB eyes, ultrasonics
byte pwm_active     = 1;          //PCA9685 servo driver - without this Nova cannot move
byte ps2_active     = 1;          //PS2 wireless remote
byte serial_active  = 1;          //typed commands over USB serial
byte mpu_active     = 0;          //MPU6050 IMU for self-levelling
byte rgb_active     = 1;          //NeoPixel eyes (driven via the slave)
byte oled_active    = 1;          //SSD1331 display (driven via the slave)
byte pir_active     = 0;          //three PIR motion sensors
byte uss_active     = 0;          //two ultrasonic distance sensors (via the slave)
byte amp_active     = 0;          //ACS712 current sensor / stall detection
byte batt_active    = 0;          //battery voltage monitoring and low-battery halt
byte buzz_active    = 0;          //piezo buzzer sound effects
byte melody_active  = 1;          //musical startup tunes
byte mp3_active     = 1;          //DFPlayer Mini MP3 module
byte splash_active  = 1;          //play the full boot animation on the OLED
byte quick_boot     = 1;          //skip most boot graphics and sounds - much faster to test with


/*
   -------------------------------------------------------
   Pin map - Teensy 4.0 (master)
   -------------------------------------------------------

     pin  | used for
     -----+----------------------------------------------------------
      0   | MP3 module RX  (Teensy transmits into it)
      1   | MP3 module TX
      2   | ACS712 current sensor power control
      3   | PCA9685 Output Enable  (ACTIVE LOW - see note below)
      4   | PIR motion sensor, front
      5   | PIR motion sensor, left
      6   | PIR motion sensor, right
      7   | PS2 receiver DATA
      8   | PS2 receiver COMMAND
      9   | PS2 receiver ATTENTION / select
      10  | PS2 receiver CLOCK
      13  | onboard LED
      16  | i2c bus 2 SCL - MPU6050 and the Nano slave
      17  | i2c bus 2 SDA - MPU6050 and the Nano slave
      20  | MP3 volume potentiometer  (currently unused, see mp3_volume())
      22  | piezo buzzer
      A1  | ACS712 current sense AND battery voltage divider - see warning below

   *** THE TEENSY 4.0 IS NOT 5V TOLERANT ***
   Every input above must stay at or below 3.3V. In particular the PS2
   receiver must be powered from 3.3V, not 5V, or its DATA line will damage
   the Teensy.
*/

//onboard and bus pins
#define SDA2_PIN 17               //i2c bus 2 data  (MPU6050 + Nano slave)
#define SCL2_PIN 16               //i2c bus 2 clock (MPU6050 + Nano slave)
#define LED_PIN 13                //onboard LED

//PCA9685 servo driver
#define OE_PIN 3                  //Output Enable. ACTIVE LOW: LOW enables the
                                  //servos, HIGH cuts their drive and lets the
                                  //legs go limp. It is held HIGH through boot
                                  //until the PS2 link is up - see ps2_check().

//PS2 wireless receiver
#define PS2_DAT 7
#define PS2_CMD 8
#define PS2_SEL 9
#define PS2_CLK 10

//PIR motion sensors
#define PIR_FRONT 4
#define PIR_LEFT 5
#define PIR_RIGHT 6

//sound
#define BUZZ 22                   //piezo buzzer / speaker module
#define MP3_VOL_PIN 20            //volume potentiometer (see the DEV note in mp3_volume())

//power monitoring
//WARNING: AMP_PIN and BATT_MONITOR are deliberately the SAME analog pin. This
//robot cannot read current and battery voltage at once, so amp_active and
//batt_active should not both be enabled unless the wiring is changed.
#define AMP_PIN A1                //ACS712 current sense
#define PWR_PIN 2                 //ACS712 power control
#define BATT_MONITOR A1           //battery voltage divider

//Nano slave i2c address
#define SLAVE_ID 1


/*
   -------------------------------------------------------
   Servo driver timing
    :CAUTION - changing either value silently invalidates every calibrated
    :position in NovaServos.h, because those are raw pwm tick counts measured
    :at this exact frequency. If Nova is standing correctly, leave these alone.
   -------------------------------------------------------
*/
#define SERVO_FREQ 60             //pwm refresh rate in Hz
#define OSCIL_FREQ 25000000       //PCA9685 oscillator, measured not nominal


/*
   -------------------------------------------------------
   Battery voltage divider
    :the analog pin sees the battery through a 4.7k / 1k divider, which keeps
    :a 12V pack under the 3.3V the Teensy can accept.
    :  measured volts = reading * (3.3 / 1023) * BATT_DIVIDER_RATIO
    :If you change the resistors, change the ratio here.
   -------------------------------------------------------
*/
#define BATT_ADC_REF 3.3          //Teensy analog reference in volts
#define BATT_ADC_STEPS 1023.00    //10-bit ADC
#define BATT_DIVIDER_RATIO 5.67   //for the 4.7k / 1k divider fitted

#endif  //NOVA_CONFIG_H
