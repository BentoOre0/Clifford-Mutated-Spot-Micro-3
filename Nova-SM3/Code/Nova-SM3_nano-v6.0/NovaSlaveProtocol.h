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
 *   MASTER <-> SLAVE COMMAND PROTOCOL
 *   -----------------------------------------------------------------------
 *
 *   The Teensy (master) and the Nano (slave) talk over i2c using a
 *   single-byte command protocol. Each byte is one command; a multi-byte
 *   string is simply several commands sent back to back, in order.
 *
 *   The slave keeps ONE piece of state that changes what an incoming byte
 *   means: the `serial_oled` mode flag.
 *
 *     serial_oled == 0  ->  SYSTEM mode: the byte is an LED / sensor command
 *     serial_oled == 1  ->  OLED mode:   the byte is a display command
 *
 *   The byte 'X' toggles that mode and is the only byte that means the same
 *   thing in both. On the master you never toggle by hand - use the two
 *   wrappers, which handle the mode switch for you:
 *
 *     rgb_request(...)   sends SYSTEM-mode bytes (RGB_* and USS_* below)
 *     oled_request(...)  sends OLED-mode bytes   (OLED_* below)
 *
 *   Because the constants below are string literals, adjacent ones
 *   concatenate at compile time, so a command sequence reads as a sentence
 *   and costs exactly the same flash as the raw literal it replaces:
 *
 *     rgb_request(RGB_LEFT RGB_RED  RGB_RIGHT RGB_YELLOW
 *                 RGB_SPEED_30  RGB_PATTERN_BLINK);
 *
 *   IMPORTANT: this file is duplicated byte-for-byte in Nova-SM3_nano-v6.0/.
 *   The Arduino IDE cannot share a header between two sketch folders, so the
 *   two copies MUST be kept in sync - if you change a value here, change it
 *   there too, or the two boards will disagree about what a byte means.
 *   -----------------------------------------------------------------------
*/

#ifndef NOVA_SLAVE_PROTOCOL_H
#define NOVA_SLAVE_PROTOCOL_H


/*
   -------------------------------------------------------
   Mode / lifecycle  (understood in BOTH modes)
   -------------------------------------------------------
*/
#define SLAVE_TOGGLE_MODE     "X"   //flip between SYSTEM and OLED mode; slave replies with its new mode
#define SLAVE_REBOOT_SPLASH   "Z"   //reboot the slave AND play the boot splash animation
#define SLAVE_REBOOT_QUIET    "Y"   //reboot the slave, skipping the splash (faster boot)


/*
   -------------------------------------------------------
   SYSTEM mode: ultrasonic sensors
    :these are the only commands that return a meaningful value,
    :read back through the int returned by command_slave()
   -------------------------------------------------------
*/
#define USS_READ_LEFT         "a"   //returns left sensor distance in cm
#define USS_READ_RIGHT        "b"   //returns right sensor distance in cm


/*
   -------------------------------------------------------
   SYSTEM mode: RGB eye animation
    :a full instruction is normally  <side+colour> <speed> <count> <pattern>
    :order does not matter, but the PATTERN byte is what starts the animation,
    :so it is written last by convention
   -------------------------------------------------------
*/

//which eye the next colour applies to
#define RGB_LEFT              "M"
#define RGB_RIGHT             "N"

//colour for the side most recently selected by RGB_LEFT / RGB_RIGHT
#define RGB_RED               "O"
#define RGB_GREEN             "P"
#define RGB_BLUE              "Q"
#define RGB_YELLOW            "R"
#define RGB_PURPLE            "S"
#define RGB_ORANGE            "T"
#define RGB_WHITE             "U"
#define RGB_GREY              "V"

//how many times the pattern repeats before the eyes go idle
#define RGB_COUNT_0           "s"
#define RGB_COUNT_1           "t"
#define RGB_COUNT_2           "u"
#define RGB_COUNT_3           "v"
#define RGB_COUNT_4           "w"
#define RGB_COUNT_5           "x"
#define RGB_COUNT_10          "y"
#define RGB_COUNT_25          "z"
#define RGB_COUNT_100         "A"

//pattern step interval in ms (also used as the fade resolution)
#define RGB_SPEED_30          "D"
#define RGB_SPEED_50          "E"
#define RGB_SPEED_100         "F"
#define RGB_SPEED_250         "G"
#define RGB_SPEED_500         "H"
#define RGB_SPEED_1000        "I"
#define RGB_SPEED_2500        "J"
#define RGB_SPEED_5000        "K"

//the animation itself
#define RGB_PATTERN_OFF       "p"   //all eyes dark
#define RGB_FADE_BLUE         "d"   //fade blue -> black
#define RGB_FADE_ORANGE       "e"   //fade black -> orange -> black
#define RGB_FADE_GREEN        "f"   //fade green -> black
#define RGB_FADE_YELLOW       "g"   //fade yellow -> black
#define RGB_FADE_RED          "h"   //fade red -> black
#define RGB_FADE_PURPLE       "i"   //fade purple -> black
#define RGB_SOLID_WHITE       "j"   //steady white
#define RGB_WIPE_PAIRED       "k"   //green wipe, eyes paired
#define RGB_FLOW              "l"   //continuous colour flow
#define RGB_RAINBOW           "m"   //rainbow cycle
#define RGB_PATTERN_BLINK     "n"   //blink using the selected left/right colours
#define RGB_WIPE_WAVE         "o"   //green wipe, one LED at a time


/*
   -------------------------------------------------------
   OLED mode: display screens
   -------------------------------------------------------
*/
#define OLED_WAKE             "a"   //"Grrrrr!"
#define OLED_STAY             "b"   //"Stay!"
#define OLED_DISTANCES        "c"   //live left/right ultrasonic readout
#define OLED_MARCH            "d"   //"March!"
#define OLED_INTRUDER         "e"   //"INTRUDER ALERT!"
#define OLED_ALERT_CLEAR      "f"   //"ALERT COMPLETE!"
#define OLED_MODE_1           "g"   //ps2 button set 1 selected
#define OLED_MODE_2           "h"   //ps2 button set 2 selected
#define OLED_MODE_3           "i"   //ps2 button set 3 selected
#define OLED_MODE_4           "j"   //ps2 button set 4 selected
#define OLED_HALTED           "k"   //"SYSTEM HALTED!"
#define OLED_READY            "l"   //"Ready!"
#define OLED_ALARM            "m"   //reserved for future alarm graphic
#define OLED_BATTERY_GAUGE    "n"   //draw the 5-bar gauge for the level set below
#define OLED_STAND_BACK       "o"   //"PLEASE STAND BACK!!"

//radar sweep graphic, used by follow mode to show which way Nova is heading
#define OLED_RADAR_FORWARD    "p"
#define OLED_RADAR_FWD_LEFT   "q"
#define OLED_RADAR_FWD_RIGHT  "r"
#define OLED_RADAR_LEFT       "s"
#define OLED_RADAR_RIGHT      "t"
#define OLED_RADAR_BACKWARD   "u"
#define OLED_RADAR_GREET      "v"
#define OLED_RADAR_STOP       "w"

//battery level to draw, 0 = healthiest ... 9 = critical.
//send one of these immediately BEFORE OLED_BATTERY_GAUGE, eg:
//    oled_request(OLED_BATTERY_LEVEL_9 OLED_BATTERY_GAUGE);
#define OLED_BATTERY_LEVEL_0  "0"
#define OLED_BATTERY_LEVEL_1  "1"
#define OLED_BATTERY_LEVEL_2  "2"
#define OLED_BATTERY_LEVEL_3  "3"
#define OLED_BATTERY_LEVEL_4  "4"
#define OLED_BATTERY_LEVEL_5  "5"
#define OLED_BATTERY_LEVEL_6  "6"
#define OLED_BATTERY_LEVEL_7  "7"
#define OLED_BATTERY_LEVEL_8  "8"
#define OLED_BATTERY_LEVEL_9  "9"


/*
   -------------------------------------------------------
   Using these on the slave
    :the master sends strings, so the constants above are one-character
    :strings and can be concatenated into a command sequence. The slave
    :receives one byte at a time, so it needs the character instead.
    :CMD_BYTE() converts, which keeps both boards reading the same
    :definition rather than two lists that can drift apart.
    :
    :  if (req == CMD_BYTE(OLED_WAKE)) { ... }
   -------------------------------------------------------
*/
#define CMD_BYTE(cmd) ((cmd)[0])

#endif  //NOVA_SLAVE_PROTOCOL_H
