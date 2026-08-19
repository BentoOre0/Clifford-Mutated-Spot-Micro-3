/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Typed command interface
 *
 *   The primary way to drive Nova from a PC without a remote. serial_check()
 *   collects a command, serial_command() dispatches it to a topic group.
 *   Type "h" on the serial monitor for the full list.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   Serial Commands
   -------------------------------------------------------
*/
void serial_check() {
  if (serial_active) {
    while (Serial.available()) {
      delay(2);  //delay to allow byte to arrive in input buffer
      char c = Serial.read();
      if (c != ' ' && c != '\n') {  //strip spaces and newlines
        if (c == ',') {  //end command
          serial_command(serial_input);
          serial_input="";
        } else {  //build command
          serial_input += c;
        }
      }
    }
  
    if (serial_input.length() > 0) {
      serial_command(serial_input);
      serial_input="";
    } 
  }
}


/*
   -------------------------------------------------------
   Serial Command Dispatch
    :typed commands are the way to drive Nova without a PS2 remote

    serial_check() collects characters until a comma or the end of the input
    buffer, then hands the whole word to serial_command() below.

    Commands are grouped by topic, one function per group. Each returns 1 if
    it recognised the command and 0 if it did not, so the dispatcher simply
    walks the groups in order and stops at the first match - the same
    first-match-wins behaviour the original single if/else chain had, but now
    you can find a command by topic instead of scrolling.

    To add a command: put it in whichever group it belongs to, and add a row
    to serial_print_help() so it is discoverable.
   -------------------------------------------------------
*/
void serial_command(String cmd) {
  if (!cmd.length()) return;

  if (serial_cmd_speed(cmd)) return;
  if (serial_cmd_report(cmd)) return;
  if (serial_cmd_gait(cmd)) return;
  if (serial_cmd_debug_target(cmd)) return;
  if (serial_cmd_nudge(cmd)) return;
  if (serial_cmd_pose_and_move(cmd)) return;
  if (serial_cmd_sweep(cmd)) return;
  if (serial_cmd_sensor_toggle(cmd)) return;
  if (serial_cmd_sound(cmd)) return;
  if (serial_cmd_ps2(cmd)) return;
  if (cmd == "h" || cmd == "help") { serial_print_help(); return; }

  Serial.print(cmd);
  Serial.println(F(" is not a valid command input.\nTry again, else type 'h' for help."));
}


//emergency stop and the nine speed presets.
//Remember spd is a DELAY in ms, so a bigger number is a SLOWER robot.
byte serial_cmd_speed(String cmd) {
  if (cmd == "stop" || cmd == "0") {
    if (!plotter) Serial.println(F("stop"));
    set_stop_active();
    set_home();
  } else if (cmd == "1") {
    if (!plotter) Serial.println(F("set speed 1"));
    spd = 1;
    set_speed();
  } else if (cmd == "2") {
    if (!plotter) Serial.println(F("set speed 5"));
    spd = 5;
    set_speed();
  } else if (cmd == "3") {
    if (!plotter) Serial.println(F("set speed 10"));
    spd = 10;
    set_speed();
  } else if (cmd == "4") {
    if (!plotter) Serial.println(F("set speed 15"));
    spd = 15;
    set_speed();
  } else if (cmd == "5") {
    if (!plotter) Serial.println(F("set speed 20"));
    spd = 20;
    set_speed();
  } else if (cmd == "6") {
    if (!plotter) Serial.println(F("set speed 25"));
    spd = 25;
    set_speed();
  } else if (cmd == "7") {
    if (!plotter) Serial.println(F("set speed 30"));
    spd = 30;
    set_speed();
  } else if (cmd == "8") {
    if (!plotter) Serial.println(F("set speed 35"));
    spd = 35;
    set_speed();
  } else if (cmd == "9") {
    if (!plotter) Serial.println(F("set speed 40"));
    spd = 40;
    set_speed();
  } else {
    return 0;
  }
  return 1;
}


//read-only diagnostics: dump the tuning variables, or run the
//OLED / RGB self-tests on the slave board.
byte serial_cmd_report(String cmd) {
  if (cmd == "vars" || cmd == "v") {
    if (!plotter) {
      Serial.println();
      Serial.println(F("---------------------------------------"));
      Serial.println(F("VARS:"));
      Serial.println(F("---------------------------------------"));
      Serial.print(F("\tspd:\t\t\t"));Serial.println(spd);
      Serial.print(F("\tspd_factor:\t\t"));Serial.println(spd_factor);
      Serial.print(F("\tmove_steps:\t\t"));Serial.println(move_steps);
      Serial.print(F("\tx_dir:\t\t\t"));Serial.println(x_dir);
      Serial.print(F("\ty_dir:\t\t\t"));Serial.println(y_dir);
      Serial.print(F("\tz_dir:\t\t\t"));Serial.println(z_dir);
      Serial.print(F("\tstep_weight_factor:\t"));Serial.println(step_weight_factor);
      Serial.print(F("\tstep_height_factor:\t"));Serial.println(step_height_factor);
      Serial.print(F("\tdebug_servo:\t\t"));Serial.println(debug_servo);
      Serial.print(F("\tdebug_leg:\t\t"));Serial.println(debug_leg);
      Serial.print(F("\tvolume:\t\t\t"));Serial.println(sound_vol);
      Serial.println(F("---------------------------------------"));
      Serial.println();
      Serial.println();

      Serial.println(F("---------------------------------------"));
      Serial.println(F("RAMP VARS:"));
      Serial.println(F("---------------------------------------"));
      Serial.print(F("\tspd:\t\t\t")); Serial.println(servoRamp[debug_servo][0]);
      Serial.print(F("\tdist:\t\t\t")); Serial.println(servoRamp[debug_servo][1]);
      Serial.print(F("\tup spd:\t\t\t")); Serial.println(servoRamp[debug_servo][2]);
      Serial.print(F("\tup dist:\t\t")); Serial.println(servoRamp[debug_servo][3]);
      Serial.print(F("\tup spd inc:\t\t")); Serial.println(servoRamp[debug_servo][4]);
      Serial.print(F("\tdn spd:\t\t\t")); Serial.println(servoRamp[debug_servo][5]);
      Serial.print(F("\tdn dist:\t\t")); Serial.println(servoRamp[debug_servo][6]);
      Serial.print(F("\tdn spd inc:\t\t")); Serial.println(servoRamp[debug_servo][7]);
      Serial.println(F("---------------------------------------"));
      Serial.println();
      Serial.println();
    }
  } else if (cmd == "oled" || cmd == "o") {
    if (oled_active) {
      if (!plotter) Serial.println(F("test OLED begin"));
      test_oled();
      if (!plotter) Serial.println(F("test OLED end"));
    } else {
      if (!plotter) Serial.println(F("OLED inactive"));
    }
  } else if (cmd == "rgb") {
    if (!plotter) Serial.println(F("test rgb begin"));
    test_rgb();
    if (!plotter) Serial.println(F("test rgb end"));
  } else {
    return 0;
  }
  return 1;
}


//start or stop a walking gait, and the stand/home/wake poses that
//bracket one.
byte serial_cmd_gait(String cmd) {
  if (cmd == "trot" || cmd == "t") {
//DEVWORK
    if (!plotter) Serial.println(F("trot"));
    spd = 5;
    set_speed();
    move_steps = 35;
    x_dir = 0;
    move_trot = 1;
  } else if (cmd == "march" || cmd == "m") {
//DEVWORK
    if (!plotter) Serial.println(F("march"));        
    spd = 5;
    set_speed();
    y_dir = 0;
    x_dir = 0;
    z_dir = 0;
    step_weight_factor = 1.20;
    move_steps = 25;
    if (mpu_is_active) mpu_active = 0;
    move_march = 1;
  } else if (cmd == "stay" || cmd == "s") {
    if (!plotter) Serial.println(F("stay"));
    set_stay();
  } else if (cmd == "home") {
    if (mpu_is_active) {
      if (mpu_active) {
        if (!plotter) Serial.println(F("mpu off!"));
        mpu_active = 0;
        if (!plotter) Serial.println(F("set_home"));
        set_home();
      } else {
        if (!plotter) Serial.println(F("mpu on!"));
        mpu_active = 1;
//            if (!plotter) Serial.print(F("mpu roll/pitch: "));Serial.print(mpu_mroll);Serial.print(F(" / "));Serial.println(mpu_mpitch);
//            set_axis(mpu_mroll, mpu_mpitch);
      }
    } else {
      if (!plotter) Serial.println(F("set_home"));
      set_home();
    }
  } else if (cmd == "wake" || cmd == "w") {
    if (!plotter) Serial.println(F("wake"));
    if (!activeServo[RFF] && !activeServo[LFF] && !activeServo[RRF] && !activeServo[LRF]) {
      spd = 1;
      set_speed();
      move_loops = 2;
      move_switch = 2;
      for (int i = 0; i < TOTAL_SERVOS; i++) {
        servoPos[i] = servoHome[i];
      }
      move_wake = 1;
    }
  } else {
    return 0;
  }
  return 1;
}


//pick what the "servo" / "leg" sweep tests exercise, and toggle the
//PIR follow-me behaviour.
byte serial_cmd_debug_target(String cmd) {
  if (cmd == "servo") {
    if (!plotter) { Serial.print("debug servo ");Serial.println(debug_servo); }
    debug_loops2 = debug_loops;
    move_servo = 1;
  } else if (cmd == "leg") {
    if (!plotter) { Serial.print("debug leg ");Serial.println(debug_leg); }
    debug_loops2 = debug_loops;
    move_leg = 1;
  } else if (cmd == "foff") {
    if (!plotter) { Serial.println("debug pir follow off"); }
    move_follow = 0;
    if (mp3_active) {
      mp3_play(17);
      delay(1500);
    } else if (buzz_active) {
      for (int b = 3; b > 0; b--) {
        tone(BUZZ, 2000);
        delay(100);
        noTone(BUZZ);
        delay(100);
      }
      noTone(BUZZ);         
    }
    if (mpu_is_active) mpu_active = 1;
    if (uss_is_active) uss_active = 1;
  } else if (cmd == "fon") {
    if (!plotter) { Serial.println("debug pir follow on"); }
    move_follow = 1;
    if (mp3_active) {
      mp3_play(16);
      delay(1500);
    } else if (buzz_active) {
      for (int b = 3; b > 0; b--) {
        tone(BUZZ, 1000);
        delay(100);
        noTone(BUZZ);
        delay(100);
      }
      noTone(BUZZ);         
    }
    if (mpu_is_active) mpu_active = 0;
    if (uss_is_active) uss_active = 0;
  } else {
    return 0;
  }
  return 1;
}


//step one tuning variable up or down by a fixed increment. Handy while
//the robot is moving, to feel what each variable actually changes.
byte serial_cmd_nudge(String cmd) {
  if (cmd == "ms-") {
    if (!plotter) Serial.print(F("move_steps -5: "));
    if (move_steps > move_steps_min) {
      move_steps -= 5;
    }
    if (!plotter) Serial.println(move_steps);
  } else if (cmd == "ms+") {
    if (!plotter) Serial.println(F("move_steps +5: "));
    if (move_steps < move_steps_max) {
      move_steps += 5;
    }
    if (!plotter) Serial.println(move_steps);
  } else if (cmd == "y-") {
    if (!plotter) Serial.print(F("y_dir -5: "));
    if (y_dir > move_y_steps[0]) {
      y_dir -= 5;
    }
    if (!plotter) Serial.println(y_dir);
  } else if (cmd == "y+") {
    if (!plotter) Serial.print(F("y_dir +5: "));
    if (y_dir < move_y_steps[1]) {
      y_dir += 5;
    }
    if (!plotter) Serial.println(y_dir);
  } else if (cmd == "x-") {
    if (!plotter) Serial.print(F("x_dir -5: "));
    if (x_dir > move_x_steps[0]) {
      x_dir -= 5;
    }
    if (!plotter) Serial.println(x_dir);
  } else if (cmd == "x+") {
    if (!plotter) Serial.print(F("x_dir +5: "));
    if (x_dir < move_x_steps[1]) {
      x_dir += 5;
    }
    if (!plotter) Serial.println(x_dir);
  } else if (cmd == "z-") {
    if (!plotter) Serial.print(F("z_dir -5: "));
    if (z_dir > move_z_steps[0]) {
      z_dir -= 5;
    }
    if (!plotter) Serial.println(z_dir);
  } else if (cmd == "z+") {
    if (!plotter) Serial.print(F("z_dir +5: "));
    if (z_dir < move_z_steps[1]) {
      z_dir += 5;
    }
    if (!plotter) Serial.println(z_dir);
  } else if (cmd == "s-") {
    if (debug_servo > 0) {
      debug_servo--;
    }
    if (!plotter) { Serial.print("set debug servo ");Serial.println(debug_servo); }
  } else if (cmd == "s+") {
    if (debug_servo < (TOTAL_SERVOS-1)) {
      debug_servo++;
    }
    if (!plotter) { Serial.print("set debug servo ");Serial.println(debug_servo); }
  } else if (cmd == "l-") {
    if (debug_leg > 0) {
      debug_leg--;
    }
    if (!plotter) { Serial.print("set debug leg ");Serial.println(debug_leg); }
  } else if (cmd == "l+") {
    if (debug_leg < (TOTAL_LEGS-1)) {
      debug_leg++;
    }
    if (!plotter) { Serial.print("set debug leg ");Serial.println(debug_leg); }
  } else {
    return 0;
  }
  return 1;
}


//the fixed poses (sit / kneel / crouch / lay) and the one-shot body
//movements, plus the scripted demo.
byte serial_cmd_pose_and_move(String cmd) {
  if (cmd == "demo") {
    if (!plotter) Serial.println(F("demo"));
    run_demo();
  } else if (cmd == "forw" || cmd == "f") {
    if (!plotter) Serial.println(F("march_forward"));
    spd = 5;
    set_speed();
    y_dir = 10;
    x_dir = 0;
    z_dir = 0;
    step_weight_factor = 1.20;
    move_steps = 25;
    move_march = 1;
  } else if (cmd == "back" || cmd == "b") {
    if (!plotter) Serial.println(F("march_backward"));
    spd = 5;
    set_speed();
    y_dir = -10;
    x_dir = 0;
    z_dir = 0;
    step_weight_factor = 1.20;
    move_steps = 30;
    move_march = 1;
  } else if (cmd == "sit") {
    if (!plotter) Serial.println(F("sit"));
    set_sit();
  } else if (cmd == "kneel") {
    if (!plotter) Serial.println(F("kneel"));
    set_kneel();
  } else if (cmd == "crouch" || cmd == "c") {
    if (!plotter) Serial.println(F("crouch"));
    set_crouch();
  } else if (cmd == "lay") {
    if (!plotter) Serial.println(F("lay"));
    set_lay();
  } else if (cmd == "roll") {
    if (!plotter) Serial.println(F("roll"));
    move_steps = 30;
    x_dir = 0;
    move_roll = 1;
  } else if (cmd == "pitch") {
    if (!plotter) Serial.println(F("pitch"));
    move_steps = 30;
    x_dir = 0;
    move_pitch = 1;
  } else if (cmd == "rollb") {
    if (!plotter) Serial.println(F("roll_body"));
    move_steps = 30;
    x_dir = 0;
    move_roll_body = 1;
  } else if (cmd == "pitchb") {
    if (!plotter) Serial.println(F("pitch_body"));
    move_steps = 30;
    x_dir = 0;
    move_pitch_body = 1;
  } else if (cmd == "wman") {
    if (!plotter) Serial.println(F("wman"));
    spd = 3;
    set_speed();
    move_wman = 1;
  } else if (cmd == "y") {
    if (!plotter) Serial.println(F("y_axis"));
    move_y_axis = 1;
    y_axis();
  } else if (cmd == "x") {
    if (!plotter) Serial.println(F("x_axis"));
    move_x_axis = 1;
    x_axis();
  } else if (cmd == "framp" || cmd == "fr") {
    if (!plotter) Serial.println(F("move_forward"));
    spd = 5;
    ramp_dist = 0.15;
    ramp_spd = 1.25;
    use_ramp = 1;
    x_dir = 0;
    move_forward = 1;
  } else {
    return 0;
  }
  return 1;
}


//sweep a whole joint group between its travel limits. This is the
//quickest way to confirm every servo of one type still moves freely.
byte serial_cmd_sweep(String cmd) {
  if (cmd == "st") {
    if (!plotter) Serial.println(F("sweep tibias"));
    use_ramp = 0;
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (is_tibia(i)) {
        set_sweep(i, servoSpeed[i], servoLimit[i][0], servoLimit[i][1], 1);
      }
    }
  } else if (cmd == "rst") {
    if (!plotter) Serial.println(F("ramp sweep tibia"));
    use_ramp = 1;
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (is_tibia(i)) {
        set_sweep(i, servoSpeed[i], servoLimit[i][0], servoLimit[i][1], 1);
        set_ramp(i, servoSpeed[i], 0, 0, 0, 0);
      }
    }
  } else if (cmd == "sf") {
    if (!plotter) Serial.println(F("sweep femurs"));
    use_ramp = 0;
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (is_femur(i)) {
        set_sweep(i, servoSpeed[i], servoLimit[i][0], servoLimit[i][1], 1);
      }
    }
  } else if (cmd == "rsf") {
    if (!plotter) Serial.println(F("ramp sweep femur"));
    use_ramp = 1;
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (is_femur(i)) {
        set_sweep(i, servoSpeed[i], servoLimit[i][0], servoLimit[i][1], 1);
        set_ramp(i, servoSpeed[i], 0, 0, 0, 0);
      }
    }
  } else if (cmd == "sc") {
    if (!plotter) Serial.println(F("sweep coaxes"));
    use_ramp = 0;
    for (int i = 0; i < TOTAL_SERVOS; i++) {
      if (!is_femur(i) && !is_tibia(i)) {
        set_sweep(i, servoSpeed[i], servoLimit[i][0], servoLimit[i][1], 1);
      }
    }
  } else if (cmd == "lr") {
      if (!plotter) Serial.println(F("look right"));
      spd = 1;
      set_speed();
      move_loops = 1;
      move_steps = 30;
      move_look_right = 1;
  } else if (cmd == "ll") {
      if (!plotter) Serial.println(F("look left"));
      spd = 1;
      set_speed();
      move_loops = 1;
      move_steps = 30;
      move_look_left = 1;
  } else {
    return 0;
  }
  return 1;
}


//turn the IMU and the two sensor sets on and off at runtime. Each only
//responds if that subsystem was enabled at boot - see NovaConfig.h.
byte serial_cmd_sensor_toggle(String cmd) {
  if (cmd == "mpu") {
    if (mpu_active) {
      if (!plotter) Serial.println(F("mpu off"));
      mpu_active = 0;
    } else if(mpu_is_active) {
      if (!plotter) Serial.println(F("mpu on"));
      mpu_active = 1;
    } else {
      if (!plotter) Serial.println(F("mpu inactive"));
    }
  } else if (cmd == "uss") {
    if (uss_active) {
      if (!plotter) Serial.println(F("us sensors off"));
      uss_active = 0;
    } else if(uss_is_active) {
      if (!plotter) Serial.println(F("us sensors on"));
      uss_active = 1;
      int dist_rt = command_slave(USS_READ_RIGHT);
      int dist_lt = command_slave(USS_READ_LEFT);
      if (oled_active) {
        oled_request(OLED_DISTANCES);
      }
      if (!plotter) {
        Serial.print(F("R: "));Serial.print(dist_rt);Serial.print(F(" L: "));Serial.println(dist_lt);
      }
    } else {
      if (!plotter) Serial.println(F("us sensors inactive"));
    }
  } else if (cmd == "pir") {
    if (pir_active) {
      if (!plotter) Serial.println(F("pir sensors off"));
      pir_active = 0;
    } else if(pir_is_active) {
      if (!plotter) Serial.println(F("pir sensors on"));
      pir_active = 1;
    } else {
      if (!plotter) Serial.println(F("pir sensors inactive"));
    }
  } else {
    return 0;
  }
  return 1;
}


//MP3 playback, volume, and the OLED battery gauge test.
byte serial_cmd_sound(String cmd) {
  if (cmd == "mp2") {
    if (mp3_active) {
      mp3_play(random(1,22));
    }
  } else if (cmd == "mp3") {
    if (mp3_active) {
      if (!mp3_status) {
        mp3_status = 1;
        int q = random(1,6);
        for (int i=1;i<q;i++) {
          add_to_mp3_queue(random(1,22));
        }
      }
    }
  } else if (cmd == "vup") {
    if (mp3_active) {
      if (sound_vol < 30) {
        sound_vol++;
        mp3_volume(sound_vol);
        delay(30);
        Serial.print("volume up: ");Serial.println(sound_vol);
      }
    }
  } else if (cmd == "vdn") {
    if (mp3_active) {
      if (sound_vol > 1) {
        sound_vol--;
        mp3_volume(sound_vol);
        delay(30);
        Serial.print("volume down: ");Serial.println(sound_vol);
      }
    }
  } else if (cmd == "batt") {
    if (oled_active) {
      if (!plotter) Serial.println(F("battery display test"));
      oled_request(OLED_BATTERY_GAUGE);
      delay(3000);
    } else {
      if (!plotter) Serial.println(F("oled inactive"));
    }
  } else {
    return 0;
  }
  return 1;
}


//PS2 remote diagnostics: toggle the live button report, or retry the
//handshake without power-cycling the board.
byte serial_cmd_ps2(String cmd) {
  if (cmd == "ps2") {
    if (!ps2_active) {
      if (!plotter) Serial.println(F("ps2 inactive - controller did not connect at boot, try 'ps2r'"));
    } else if (ps2_debug) {
      ps2_debug = 0;
      if (!plotter) Serial.println(F("ps2 debug off"));
    } else {
      ps2_debug = 1;
      if (!plotter) {
        Serial.println(F("ps2 debug on"));
        Serial.print(F("\tbutton set (SELECT cycles 1-4):\t"));Serial.println(ps2_select);
        Serial.print(F("\tservo output (pwm_oe):\t\t"));Serial.println(pwm_oe);
      }
    }
  } else if (cmd == "ps2r") {
    //re-run the handshake without rebooting, for a receiver that is powered
    //but did not enumerate at boot
    if (!plotter) Serial.print(F("ps2 reconnecting..."));
    if (ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false) == 0) {
      ps2_active = 1;
      if (!plotter) Serial.println(F("\t\tOK"));
    } else {
      ps2_active = 0;
      if (!plotter) Serial.println(F(" Error! check DAT 7 / CMD 8 / SEL 9 / CLK 10, and 3.3v not 5v"));
    }
  } else {
    return 0;
  }
  return 1;
}


//The command reference printed by "h". Sections mirror the serial_cmd_*
//groups above, so adding a command means adding one row in the matching
//section here. Everything the sketch accepts is listed - the previous table
//was hand-aligned with tabs and had drifted out of step with the code.
void serial_print_help_section(const char* title, const HelpRow* rows, byte count) {
  Serial.println();
  Serial.print(F("  "));
  Serial.println(title);
  for (byte i = 0; i < count; i++) {
    Serial.print(F("    "));
    Serial.print(rows[i].input);
    for (byte pad = strlen(rows[i].input); pad < 12; pad++) Serial.print(' ');
    Serial.println(rows[i].what);
  }
}


void serial_print_help() {
  static const HelpRow speed_rows[] = {
    { "0 / stop",  "stop everything and return to home" },
    { "1 .. 9",    "speed preset: 1 5 10 15 20 25 30 35 40 (smaller = faster)" },
  };
  static const HelpRow report_rows[] = {
    { "v / vars",  "print speed, step, direction and ramp variables" },
    { "o / oled",  "run the OLED display self-test" },
    { "rgb",       "run the RGB eye self-test" },
  };
  static const HelpRow gait_rows[] = {
    { "m / march", "march in place" },
    { "t / trot",  "trot in place" },
    { "f / forw",  "march forward" },
    { "b / back",  "march backward" },
    { "fr / framp","walk forward with acceleration ramping" },
    { "s / stay",  "stand still at the home pose" },
    { "w / wake",  "stand up from a resting pose" },
    { "home",      "servos to home (toggles the IMU when one is fitted)" },
  };
  static const HelpRow pose_rows[] = {
    { "sit",       "sit back on the rear legs" },
    { "kneel",     "kneel on all four" },
    { "c / crouch","crouch low" },
    { "lay",       "lay flat" },
    { "roll",      "rock side to side on the legs" },
    { "pitch",     "rock front to back on the legs" },
    { "rollb",     "rock side to side on the hips only" },
    { "pitchb",    "rock front to back on the hips only" },
    { "x",         "x-axis wiggle" },
    { "y",         "y-axis wiggle" },
    { "lr / ll",   "look right / look left" },
    { "wman",      "walking-man routine" },
    { "demo",      "run the scripted demo sequence" },
  };
  static const HelpRow nudge_rows[] = {
    { "ms- / ms+", "move_steps -5 / +5   (stride length)" },
    { "x- / x+",   "x_dir -5 / +5        (steer left / right)" },
    { "y- / y+",   "y_dir -5 / +5        (forward / backward)" },
    { "z- / z+",   "z_dir -5 / +5        (ride height)" },
  };
  static const HelpRow debug_rows[] = {
    { "servo",     "sweep the selected debug servo through its limits" },
    { "s- / s+",   "select previous / next debug servo" },
    { "leg",       "sweep the selected debug leg through its limits" },
    { "l- / l+",   "select previous / next debug leg" },
    { "st / rst",  "sweep all tibias, without / with ramping" },
    { "sf / rsf",  "sweep all femurs, without / with ramping" },
    { "sc",        "sweep all coaxes" },
  };
  static const HelpRow subsystem_rows[] = {
    { "mpu",       "MPU6050 IMU on / off" },
    { "uss",       "ultrasonic sensors on / off (also prints a reading)" },
    { "pir",       "PIR motion sensors on / off" },
    { "fon / foff","PIR follow-me mode on / off" },
    { "ps2",       "live PS2 button / stick report on / off" },
    { "ps2r",      "retry the PS2 handshake without rebooting" },
  };
  static const HelpRow sound_rows[] = {
    { "mp2",       "play one random mp3 track" },
    { "mp3",       "queue several random mp3 tracks" },
    { "vup / vdn", "volume up / down" },
    { "batt",      "OLED battery gauge test" },
  };

  Serial.println();
  Serial.println(F("  ------------------------------------------------------------------"));
  Serial.println(F("  NOVA SM3 SERIAL COMMANDS   (end each command with a comma)"));
  Serial.println(F("  ------------------------------------------------------------------"));

  serial_print_help_section("SPEED",              speed_rows,     sizeof(speed_rows)     / sizeof(HelpRow));
  serial_print_help_section("WALKING",            gait_rows,      sizeof(gait_rows)      / sizeof(HelpRow));
  serial_print_help_section("POSES AND MOVES",    pose_rows,      sizeof(pose_rows)      / sizeof(HelpRow));
  serial_print_help_section("TUNING",             nudge_rows,     sizeof(nudge_rows)     / sizeof(HelpRow));
  serial_print_help_section("SERVO TESTING",      debug_rows,     sizeof(debug_rows)     / sizeof(HelpRow));
  serial_print_help_section("SUBSYSTEMS",         subsystem_rows, sizeof(subsystem_rows) / sizeof(HelpRow));
  serial_print_help_section("SOUND",              sound_rows,     sizeof(sound_rows)     / sizeof(HelpRow));
  serial_print_help_section("DIAGNOSTICS",        report_rows,    sizeof(report_rows)    / sizeof(HelpRow));

  Serial.println();
  Serial.print(F("  currently selected debug servo "));Serial.print(debug_servo);
  Serial.print(F(", debug leg "));Serial.println(debug_leg);
  Serial.println(F("  ------------------------------------------------------------------"));
  Serial.println(F("  Type a command input or 'h' for help:"));
  Serial.println();
}


void test_oled() {
          oled_request(OLED_WAKE);
          delay(6000);

          oled_request(OLED_BATTERY_LEVEL_0 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_1 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_2 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_3 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_4 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_5 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_6 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_7 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_8 OLED_BATTERY_GAUGE);
          delay(3000);
          oled_request(OLED_BATTERY_LEVEL_9 OLED_BATTERY_GAUGE);
          delay(3000);


/*
          oled_request(OLED_DISTANCES);
          delay(3000);
          oled_request(OLED_MODE_1);
          delay(3000);
          oled_request(OLED_MARCH);
          delay(1500);
*/
}


void test_rgb() {
        rgb_request(RGB_LEFT RGB_RED RGB_RIGHT RGB_YELLOW RGB_SPEED_500 RGB_COUNT_10 RGB_PATTERN_BLINK);
        delay(3000);
        rgb_request(RGB_FADE_PURPLE);
        delay(3000);
        rgb_request(RGB_WIPE_PAIRED);
        delay(3000);
        rgb_request(RGB_FLOW);
        delay(3000);
        rgb_request(RGB_LEFT RGB_YELLOW RGB_RIGHT RGB_PURPLE RGB_SPEED_500 RGB_COUNT_2 RGB_PATTERN_BLINK);
        delay(1500);
        rgb_request(RGB_LEFT RGB_PURPLE RGB_RIGHT RGB_YELLOW RGB_SPEED_500 RGB_COUNT_2 RGB_PATTERN_BLINK);
        delay(1500);
        rgb_request(RGB_RAINBOW);
        delay(3000);
        rgb_request(RGB_PATTERN_BLINK);
        delay(3000);
        rgb_request(RGB_LEFT RGB_WHITE RGB_RIGHT RGB_WHITE RGB_SPEED_30 RGB_COUNT_25 RGB_PATTERN_BLINK);
        delay(3000);
        rgb_request(RGB_LEFT RGB_PURPLE RGB_RIGHT RGB_PURPLE RGB_SPEED_30 RGB_COUNT_25 RGB_PATTERN_BLINK);
        delay(3000);
}
