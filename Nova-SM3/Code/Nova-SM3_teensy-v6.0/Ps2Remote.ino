/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Reading the PS2 remote
 *
 *   Everything that turns controller input into movement requests. The
 *   remote has four button sets cycled with SELECT; ps2_check() is the entry
 *   point and explains how they work.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   PS2 Debug Report
    :edge-triggered dump of controller state, toggled by the 'ps2' serial command
    :unlike Tests/Test_PS2 this runs inside the live sketch, so it reports the
    :active button set (ps2_select) alongside the movement vars those sticks are
    :actually driving - which is what makes a mis-mapped control visible
    :called from ps2_check() only, so it inherits the ps2Interval poll rate
   -------------------------------------------------------
*/
void ps2_debug_report() {
  for (int i = 0; i < 16; i++) {
    if (ps2x.ButtonPressed(ps2_btn_mask[i])) {
      Serial.print(F("ps2["));Serial.print(ps2_select);
      Serial.print(F("] DOWN\t"));Serial.print(ps2_btn_name[i]);
      //L1/L2/R1/R2 report pressure on a dualshock, 0 on a clone pad
      if (ps2_btn_mask[i] == PSB_L1 || ps2_btn_mask[i] == PSB_L2 ||
          ps2_btn_mask[i] == PSB_R1 || ps2_btn_mask[i] == PSB_R2) {
        Serial.print(F("\tanalog: "));Serial.print(ps2x.Analog(ps2_btn_mask[i]));
      }
      Serial.println();
    }
    if (ps2x.ButtonReleased(ps2_btn_mask[i])) {
      Serial.print(F("ps2["));Serial.print(ps2_select);
      Serial.print(F("]   up\t"));Serial.println(ps2_btn_name[i]);
    }
  }

  int lx = ps2x.Analog(PSS_LX); int ly = ps2x.Analog(PSS_LY);
  int rx = ps2x.Analog(PSS_RX); int ry = ps2x.Analog(PSS_RY);
  byte moved = (abs(lx - 128) > ps2_deadzone || abs(ly - 128) > ps2_deadzone ||
                abs(rx - 128) > ps2_deadzone || abs(ry - 128) > ps2_deadzone);

  if (moved && (millis() - lastPS2Debug > ps2DebugInterval)) {
    lastPS2Debug = millis();
    lastPS2Beat = millis();
    Serial.print(F("ps2["));Serial.print(ps2_select);
    Serial.print(F("] stick\tLX:"));Serial.print(lx);
    Serial.print(F(" LY:"));Serial.print(ly);
    Serial.print(F("\tRX:"));Serial.print(rx);
    Serial.print(F(" RY:"));Serial.print(ry);
    //report what the current button set maps those sticks onto
    if (!pwm_oe) {
      Serial.print(F("\t(output off, sticks not mapped)"));
    } else if (ps2_select == 2) {
      Serial.print(F("\t-> x:"));Serial.print(x_dir);
      Serial.print(F(" y:"));Serial.print(y_dir);
      Serial.print(F(" z:"));Serial.print(z_dir);
      Serial.print(F(" steps:"));Serial.print(move_steps);
    } else if (ps2_select == 3) {
      Serial.print(F("\t-> steps_x:"));Serial.print(move_steps_x);
      Serial.print(F(" steps_y:"));Serial.print(move_steps_y);
      Serial.print(F(" yaw:"));Serial.print(move_steps_yaw);
    } else {
      Serial.print(F("\t(set "));Serial.print(ps2_select);Serial.print(F(" ignores sticks)"));
    }
    Serial.println();
  }

  //heartbeat, so a quiet controller reads differently from a dead poll loop
  if (!moved && (millis() - lastPS2Beat > 5000)) {
    lastPS2Beat = millis();
    Serial.print(F("ps2["));Serial.print(ps2_select);
    Serial.print(F("] idle\tsticks centred\tpwm_oe: "));Serial.println(pwm_oe);
  }
}


/*
   -------------------------------------------------------
   PS2 Check
    :read the remote and turn button state into movement requests

    This is the main way Nova is driven. It runs on its own timer
    (ps2Interval) and never blocks - a button press only sets the relevant
    move_* flag and its parameters, and loop() advances the movement itself.

    The remote has FOUR button sets, cycled with SELECT and tracked in
    `ps2_select`. Every button means something different in each set, which
    is why the handlers below are all shaped as "which button, then which
    set". Roughly:

      set 1  walking and body attitude (roll / pitch / axis wiggle)
      set 2  gait control and fixed poses (sit / kneel / crouch / lay)
      set 3  kinematics - the sticks steer body position and yaw directly
      set 4  per-joint calibration jogging (see ps2_leg_calibration)

    If a button appears to do nothing, check which set is active first: the
    `ps2` serial command turns on a live report of button presses, stick
    values and the current set.
   -------------------------------------------------------
*/
void ps2_check() {
  if (!pwm_oe) {
    //servo output is not enabled yet, so there is nothing to steer
    ps2_wait_for_connect();
    lastPS2Update = millis();
    return;
  }

  ps2x.read_gamepad(false, false);
  if (ps2_debug) ps2_debug_report();

  //the demo and funplay routines script the servos themselves; ignore the
  //remote while they run, or the two fight over the same joints
  if (!move_demo && !move_funplay) {
    ps2_button_start();
    ps2_button_select();
    ps2_read_joysticks();
    ps2_dpad();
    ps2_triggers();
    ps2_shapes();
    ps2_leg_calibration();
  }

  lastPS2Update = millis();
}


//START: set 1 and 3 just flash the eyes (their old bindings are kept
//commented out below), set 2 toggles marching, set 4 toggles follow mode
void ps2_button_start() {
  if (ps2x.Button(PSB_START)) {
    if (debug1)
      Serial.println(F("Start Pressed"));
    if (ps2_select == 1) {
      rgb_request(RGB_LEFT RGB_GREY RGB_RIGHT RGB_GREY);
/*
      if (mpu_active) {
        mpu_active = 0;
        rgb_request(RGB_COUNT_3 RGB_SPEED_250 RGB_PATTERN_BLINK);
      } else {
        mpu_active = 1;
        rgb_request(RGB_COUNT_5 RGB_SPEED_100 RGB_PATTERN_BLINK);
      }
*/
    } else if (ps2_select == 2) {
      if (debug1)
        Serial.println(F("start / stop march"));
      if (!move_march) {
        set_stop();
        spd = 3;
        set_speed();
        y_dir = 0;
        x_dir = 0;
        z_dir = 0;
        move_steps = 50;
        if (mpu_is_active) mpu_active = 0;
        move_march = 1;
        if (oled_active) {
          oled_request(OLED_MARCH);
        }
      } else {
        move_march = 0;
        if (mpu_is_active) mpu_active = 1;
        set_stop();
        y_dir = 0;
        x_dir = 0;
        z_dir = 0;
        if (oled_active) {
          oled_request(OLED_MARCH);
        }
      }
    } else if (ps2_select == 3) {
      rgb_request(RGB_LEFT RGB_GREY RGB_RIGHT RGB_GREY);

/*
      if (uss_active) {
        uss_active = 0;
        rgb_request(RGB_COUNT_3 RGB_SPEED_250 RGB_PATTERN_BLINK);
      } else {
        uss_active = 1;
        rgb_request(RGB_COUNT_5 RGB_SPEED_100 RGB_PATTERN_BLINK);
      }

      if (pir_halt) {
        pir_halt = 0;
        rgb_request(RGB_COUNT_3 RGB_SPEED_250 RGB_PATTERN_BLINK);
      } else {
        pir_halt = 1;
        rgb_request(RGB_COUNT_5 RGB_SPEED_100 RGB_PATTERN_BLINK);
      }
*/
    } else if (ps2_select == 4) {
//          run_demo();
      rgb_request(RGB_LEFT RGB_GREY RGB_RIGHT RGB_GREY);

      if (!move_follow) {
        spd = 3;
        set_speed();
        move_follow = 1;
        if (buzz_active) {
          for (int b = 3; b > 0; b--) {
            tone(BUZZ, 1000);          
            delay(100);  
            noTone(BUZZ);         
            delay(100);  
          }
          noTone(BUZZ);         
        }
      } else {
        move_follow = 0;
        if (buzz_active) {
          for (int b = 2; b > 0; b--) {
            tone(BUZZ, 2000);          
            delay(100);  
            noTone(BUZZ);         
            delay(100);  
          }
          noTone(BUZZ);         
        }
      }

    }
  }
}


//SELECT: cycle the active button set 1 -> 2 -> 3 -> 4 -> 1.
//Fires on RELEASE so holding SELECT does not race through all four.
//The eyes, the buzzer and the OLED all report the new set.
void ps2_button_select() {
  if (ps2x.ButtonReleased(PSB_SELECT)) {
    (ps2_select < 4) ? ps2_select++ : ps2_select = 1;
    if (rgb_active) {
      rgb_request(RGB_LEFT RGB_BLUE RGB_RIGHT RGB_BLUE);

      if (ps2_select == 1) {
        rgb_request(RGB_COUNT_1 RGB_SPEED_50 RGB_PATTERN_BLINK);
      } else if (ps2_select == 2) {
        rgb_request(RGB_COUNT_2 RGB_SPEED_50 RGB_PATTERN_BLINK);
      } else if (ps2_select == 3) {
        rgb_request(RGB_COUNT_3 RGB_SPEED_50 RGB_PATTERN_BLINK);
      } else if (ps2_select == 4) {
        rgb_request(RGB_COUNT_4 RGB_SPEED_50 RGB_PATTERN_BLINK);
      }
    }
    if (buzz_active) {
      for (int b = ps2_select; b > 0; b--) {
        tone(BUZZ, 2000);          
        delay(70);  
        noTone(BUZZ);         
        delay(70);  
      }
      noTone(BUZZ);         
    }
    if (oled_active) {
      if (ps2_select == 1) {
        oled_request(OLED_MODE_1);
      } else if (ps2_select == 2) {
        oled_request(OLED_MODE_2);
      } else if (ps2_select == 3) {
        oled_request(OLED_MODE_3);
      } else if (ps2_select == 4) {
        oled_request(OLED_MODE_4);
      }
    }
    if (debug1) {
      Serial.print(F("\tSelected ")); Serial.println(ps2_select);
    }
  }
}


//analog sticks. Which movement variables they drive depends entirely on
//the active button set, which is why a stick that "does nothing" is
//usually just the wrong set - the ps2 serial command reports which.
void ps2_read_joysticks() {
  //gait joysticks
  if (ps2_select == 2) {
    y_dir = mapfloat(ps2x.Analog(PSS_LY), 0, 255, y_dir_steps[1], y_dir_steps[0]);
    x_dir = mapfloat(ps2x.Analog(PSS_LX), 0, 255, x_dir_steps[1], x_dir_steps[0]);
    z_dir = mapfloat(ps2x.Analog(PSS_RY), 0, 255, z_dir_steps[1], z_dir_steps[0]);

    //set move_steps to min 40 to maintain march-in-place
    //DEVNOTE: make this switchable via PS2
    move_steps = map(ps2x.Analog(PSS_RX), 0, 255, 40, move_steps_max + (move_steps_max * 0.2));
  }

  //kinematics joysticks
  if (ps2_select == 3) {
    //holding R3 freezes body attitude, so the left stick can be moved
    //without the body following it
    if (!ps2x.Button(PSB_R3)) {
      move_steps_y = map(ps2x.Analog(PSS_LY), 0, 255, (move_steps_max * 1.4), (move_steps_min * 1.4));
      move_pitch_y = 1;
      if (debug1 && move_steps_y) {
        Serial.print(F("move pitch_y "));Serial.println(move_steps_y);
      }

      move_steps_x = map(ps2x.Analog(PSS_LX), 0, 255, (move_steps_max * 1.4), (move_steps_min * 1.4));
      move_roll_x = 1;
      if (debug1 && move_steps_x) {
        Serial.print(F("move roll_x "));Serial.println(move_steps_x);
      }
    }

    if (ps2x.Button(PSB_L3)) {
      //move yaw while button pressed/held
      move_steps_yaw = map(ps2x.Analog(PSS_RX), 0, 255, (move_steps_max * 1.4), (move_steps_min * 1.4));
      if (move_steps_yaw > 2 || move_steps_yaw < -2) {
        move_yaw = 1;
        if (debug1)
          Serial.println(F("move yaw"));
      } else {
        move_yaw = 0;
      }

      //move in y while button pressed/held
      move_steps_ky = map(ps2x.Analog(PSS_RY), 0, 255, (move_steps_min * 1.4), (move_steps_max * 1.4));
      if (move_steps_ky > 2 || move_steps_ky < -2) {
        move_kin_y = 1;
        if (debug1)
          Serial.println(F("move kin_y"));
      } else {
        move_kin_y = 0;
      }
    } else {
      move_steps_yaw_x = map(ps2x.Analog(PSS_RX), 0, 255, (move_steps_max * .5), (move_steps_min * .5));
      if (move_steps_yaw_x > 2 || move_steps_yaw_x < -2) {
        move_yaw_x = 1;
        if (debug1)
          Serial.println(F("move yaw_x"));
      } else {
        move_yaw_x = 0;
      }

      move_steps_yaw_y = map(ps2x.Analog(PSS_RY), 0, 255, (move_steps_max * .8), (move_steps_min * .8));
      if (move_steps_yaw_y > 2 || move_steps_yaw_y < -2) {
        move_yaw_y = 1;
        if (debug1)
          Serial.println(F("move yaw_y"));
      } else {
        move_yaw_y = 0;
      }
    }  
  }
}


//D-pad: walk (set 1), trot (set 3), and stay/wake on DOWN in every set
void ps2_dpad() {
  if (ps2x.Button(PSB_PAD_UP)) {
    if (ps2_select == 1) {
      if (debug1)
        Serial.println(F("forward"));
      if (!move_forward) {
        set_stop();
        if (rgb_active) {
          rgb_request(RGB_SPEED_100 RGB_FADE_GREEN);
        }
        
        if (mpu_is_active) mpu_active = 0;
        move_march = 1;
        spd = 12;
        set_speed();
        move_forward = 1;
      }
    } else if (ps2_select == 3) {
      if (!move_trot) {
        set_stop();
        move_trot = 1;
        if (debug1)
          Serial.println(F("move trot"));
      }
      x_dir = map(ps2x.Analog(PSS_RX), 0, 255, move_steps_min / 4, move_steps_max / 4);
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_steps_max / 2, move_steps_min / 2);
      if (debug2) {
        Serial.print(F("x dir: ")); Serial.print(x_dir);
        Serial.print(F("\tmove steps: ")); Serial.println(move_steps);
      }
    }
  } else if (ps2x.ButtonReleased(PSB_PAD_UP)) {
    if (ps2_select == 1 || ps2_select == 2 || ps2_select == 3) {
      if (move_forward) {
        move_forward = 0;
        if (debug1)
          Serial.println(F("stop forward"));
      }
      if (move_march) {
        if (mpu_is_active) mpu_active = 1;
        move_march = 0;
      }
      if (move_trot) {
        move_trot = 0;
      }
    }
  }
  
  if (ps2x.Button(PSB_PAD_RIGHT)) {
    if (ps2_select == 1) {
      if (!move_right) {
        move_right = 1;
        if (debug1)
          Serial.println(F("move right"));
      }
      x_dir = map(ps2x.Analog(PSS_RX), 0, 255, move_x_steps[0], move_x_steps[1]);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    }
  } else if (ps2x.ButtonReleased(PSB_PAD_RIGHT)) {
    if (ps2_select == 1) {
      if (move_right) {
        move_right = 0;
        if (debug1)
          Serial.println(F("stop move right"));
      }
    }
  }

  if (ps2x.Button(PSB_PAD_LEFT)) {
    if (ps2_select == 1) {
      if (!move_left) {
        move_left = 1;
        if (debug1)
          Serial.println(F("move left"));
      }
      x_dir = map(ps2x.Analog(PSS_RX), 0, 255, move_x_steps[1], move_x_steps[0]);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    }
  } else if (ps2x.ButtonReleased(PSB_PAD_LEFT)) {
    if (ps2_select == 1) {
      if (move_left) {
        move_left = 0;
        if (debug1)
          Serial.println(F("stop move left"));
      }
    }
  }  
  
  if (ps2x.ButtonPressed(PSB_PAD_DOWN)) {
    if (ps2_select == 1 || ps2_select == 2 || ps2_select == 3 || ps2_select == 4) {
      if (!ps2x.Button(PSB_L1) && !ps2x.Button(PSB_L2) && !ps2x.Button(PSB_R1) && !ps2x.Button(PSB_R2)) {
        set_stop();
        if (debug1)
          Serial.println(F("stay"));
        if (rgb_active) {
          rgb_request(RGB_SPEED_500 RGB_FADE_YELLOW);
        }
        set_stay();
        if (oled_active) {
          oled_request(OLED_STAY);
        }
      }
    }
  } else if (ps2x.ButtonReleased(PSB_PAD_DOWN)) {
    if (ps2_select == 1 || ps2_select == 2 || ps2_select == 3 || ps2_select == 4) {
      if (!ps2x.Button(PSB_L1) && !ps2x.Button(PSB_L2) && !ps2x.Button(PSB_R1) && !ps2x.Button(PSB_R2)) {
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
      }
    }
  }
}


//shoulder triggers: body roll/pitch while held (set 1),
//or a fixed pose - sit / kneel / crouch / lay (set 2)
void ps2_triggers() {
  //TRIGGER BUTTONS
  if (ps2x.Button(PSB_L1)) {
    if (ps2_select == 1) {
      if (!move_roll) {
        set_stop();
        move_roll = 1;
        if (debug1)
          Serial.println(F("move roll"));
      }
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_steps_max, move_steps_min);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    } else if (ps2_select == 2) {
      set_stop();
      if (debug1)
        Serial.println(F("sit"));
      if (rgb_active) {
        rgb_request(RGB_SPEED_500 RGB_FADE_PURPLE);
      }
      set_sit();
    }
  } else if (ps2x.ButtonReleased(PSB_L1)) {
    if (ps2_select == 1) {
      if (move_roll) {
        move_roll = 0;
        if (debug1)
          Serial.println(F("stop roll"));
      }
    }
  }

  if (ps2x.Button(PSB_L2)) {
    if (ps2_select == 1) {
      if (!move_pitch) {
        set_stop();
        move_pitch = 1;
        if (debug1)
          Serial.println(F("move pitch"));
      }
      x_dir = map(ps2x.Analog(PSS_RX), 0, 255, move_steps_min / 2, move_steps_max / 2);
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_steps_max / 3, move_steps_min / 3);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    } else if (ps2_select == 2) {
      set_stop();
      if (debug1)
        Serial.println(F("kneel"));
      set_kneel();
    }
  } else if (ps2x.ButtonReleased(PSB_L2)) {
    if (ps2_select == 1) {
      if (move_pitch) {
        move_pitch = 0;
        if (debug1)
          Serial.println(F("stop pitch"));
      }
    }
  }

  if (ps2x.Button(PSB_R1)) {
    if (ps2_select == 1) {
      if (!move_roll_body) {
        set_stop();
        move_roll_body = 1;
        if (debug1)
          Serial.println(F("move roll_body"));
      }
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_steps_max / 2, move_steps_min / 2);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    } else if (ps2_select == 2) {
      set_stop();
      if (debug1)
        Serial.println(F("crouch"));
      if (rgb_active) {
        rgb_request(RGB_SPEED_500 RGB_FADE_RED);
      }
      set_crouch();
    }
  } else if (ps2x.ButtonReleased(PSB_R1)) {
    //BUGFIX (v6.0): this tested PSB_R2, so move_roll_body was never cleared
    //when R1 was released - the body kept rolling until something else called
    //set_stop(). Releasing R2 cleared it instead, which also stole R2's own
    //release. Every other trigger clears the flag its own button set.
    if (ps2_select == 1) {
      if (move_roll_body) {
        move_roll_body = 0;
        if (debug1)
          Serial.println(F("stop roll_body"));
      }
    }
  }

  if (ps2x.Button(PSB_R2)) {
    if (ps2_select == 1) {
      if (!move_pitch_body) {
        set_stop();
        move_pitch_body = 1;
        if (debug1)
          Serial.println(F("move pitch_body"));
      }
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_steps_max, move_steps_min);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    } else if (ps2_select == 2) {
      set_stop();
      if (debug1)
        Serial.println(F("lay"));
      if (rgb_active) {
        rgb_request(RGB_SPEED_5000 RGB_FADE_BLUE);
      }
      set_lay();
    }
  } else if (ps2x.ButtonReleased(PSB_R2)) {
    if (ps2_select == 1) {
      if (move_pitch_body) {
        move_pitch_body = 0;
        if (debug1)
          Serial.println(F("stop pitch_body"));
      }
    }
  }
}


//shape buttons: x/y axis wiggle and the walking-man routine,
//plus CIRCLE / SQUARE which nudge the global speed in every set
void ps2_shapes() {
  //SHAPE BUTTONS
  if (ps2x.ButtonPressed(PSB_TRIANGLE)) {
    if (ps2_select == 1) {
      set_stop();
      if (!move_y_axis) {
        move_y_axis = 1;
        if (debug1)
          Serial.println(F("move y_axis"));
      }
    } else if (ps2_select == 2) {
      set_stop();
      if (!move_wman) {
        move_wman = 1;
        if (debug1)
          Serial.println(F("move wman"));
      }
    }
  } else if (ps2x.ButtonReleased(PSB_TRIANGLE)) {
    if (ps2_select == 1) {
      if (move_y_axis) {
        move_y_axis = 0;
        if (debug1)
          Serial.println(F("stop y_axis"));
      }
    } else if (ps2_select == 2) {
      if (move_wman) {
        move_wman = 0;
        if (debug1)
          Serial.println(F("stop wman"));
      }
    }
  }

  //poll steps stick
  if (ps2x.Button(PSB_TRIANGLE)) {
    if (ps2_select == 1) {
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_y_steps[0], move_y_steps[1]);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.print(move_steps);
      }
      if (move_steps < 0) {
        move_steps = map(move_steps, -150, 150, move_x_steps[0], move_x_steps[1]);
      }
      if (debug2) {
        Serial.print(F(" / ")); Serial.println(move_steps);
      }
    }
  }
  
  if (ps2x.ButtonPressed(PSB_CROSS)) {
    if (ps2_select == 1) {
      set_stop();
      if (!move_x_axis) {
        move_x_axis = 1;
        if (debug1)
          Serial.println(F("move x_axis"));
      }
    }
  } else if (ps2x.ButtonReleased(PSB_CROSS)) {
    if (ps2_select == 1) {
      if (move_x_axis) {
        move_x_axis = 0;
        if (debug1)
          Serial.println(F("stop x_axis"));
      }
    }
  }

  //poll steps stick
  if (ps2x.Button(PSB_CROSS)) {
    if (ps2_select == 1) {
      move_steps = map(ps2x.Analog(PSS_RY), 0, 255, move_x_steps[0], move_x_steps[1]);
      if (debug2) {
        Serial.print(F("move steps: ")); Serial.println(move_steps);
      }
    }
  }
  
  if (ps2x.ButtonPressed(PSB_CIRCLE)) {
    if (debug1) {
      Serial.println(F("Circle"));
      Serial.print(F("speed up +1 : "));
    }
    spd -= 1;
    if (spd < max_spd) spd = max_spd;
    set_speed();
    if (debug1) {
      Serial.println(spd);
    }
  }

  if (ps2x.ButtonPressed(PSB_SQUARE)) {
    if (debug1) {
      Serial.println(F("Square"));
      Serial.print(F("speed down -1 : "));
    }
    spd += 1;
    if (spd > min_spd) spd = min_spd;
    set_speed();
    if (debug1)
      Serial.println(spd);
  }
}


//button set 4 only: drive one joint at a time, for checking travel
//limits on the bench. Hold a shoulder button to pick the leg, hold a
//D-pad direction to pick the joint, then TRIANGLE / CROSS to nudge it.
void ps2_leg_calibration() {
  if (ps2_select != 4) return;

  //RIGHT FRONT leg, held with R1
  if (ps2x.Button(PSB_R1)) {
    if (ps2x.Button(PSB_PAD_UP)) {
      ps2_jog_joint(RF, RFC, "RFC");
    } else if (ps2x.Button(PSB_PAD_RIGHT)) {
      ps2_jog_joint(RF, RFF, "RFF");
    } else if (ps2x.Button(PSB_PAD_DOWN)) {
      ps2_jog_joint(RF, RFT, "RFT");
    } else if (ps2x.Button(PSB_PAD_LEFT)) {
      //LEFT is the odd one out: instead of a single joint it moves every
      //tibia together, tracked by the right stick, for levelling the body
      if (!activeServo[RFT]) {
        move_steps = ps2_jog_stick_steps();
        ps2_jog_all_legs(JOINT_TIBIA);
      }
    } else if (ps2x.ButtonReleased(PSB_PAD_UP) || ps2x.ButtonReleased(PSB_PAD_RIGHT) || ps2x.ButtonReleased(PSB_PAD_LEFT) || ps2x.ButtonReleased(PSB_PAD_DOWN)) {
      set_stop_active();
    }
  }

  //LEFT FRONT leg, held with L1
  if (ps2x.Button(PSB_L1)) {
    if (ps2x.Button(PSB_PAD_UP)) {
      ps2_jog_joint(LF, LFC, "LFC");
    } else if (ps2x.Button(PSB_PAD_RIGHT)) {
      ps2_jog_joint(LF, LFF, "LFF");
    } else if (ps2x.Button(PSB_PAD_DOWN)) {
      ps2_jog_joint(LF, LFT, "LFT");
    } else if (ps2x.Button(PSB_PAD_LEFT)) {
      //NOTE: this bare increment only survives while the guard below is false
      //(ie. while the rear-right coxa is still moving). It looks accidental,
      //but it is preserved here because removing it would change behaviour.
      move_steps++;
      //the guard names RRC while the move is all four coaxes - also preserved
      if (!activeServo[RRC]) {
        move_steps = ps2_jog_stick_steps();
        ps2_jog_all_legs(JOINT_COXA);
      }
    }
  }

  //RIGHT REAR leg, held with R2
  if (ps2x.Button(PSB_R2)) {
    if (ps2x.Button(PSB_PAD_UP)) {
      ps2_jog_joint(RR, RRC, "RRC");
    } else if (ps2x.Button(PSB_PAD_RIGHT)) {
      ps2_jog_joint(RR, RRF, "RRF");
    } else if (ps2x.Button(PSB_PAD_DOWN)) {
      ps2_jog_joint(RR, RRT, "RRT");
    } else if (ps2x.Button(PSB_PAD_LEFT)) {
      //tibias and coaxes together - raises the body and squares it up at once
      if (!activeServo[RFC] && !activeServo[RFT]) {
        move_steps = ps2_jog_stick_steps();
        if (debug1) {
          Serial.print(F("T/C move steps: ")); Serial.println(move_steps);
        }
        ps2_jog_all_legs(JOINT_TIBIA);
        ps2_jog_all_legs(JOINT_COXA);
      }
    }
  }

  //LEFT REAR leg, held with L2
  if (ps2x.Button(PSB_L2)) {
    if (ps2x.Button(PSB_PAD_UP)) {
      ps2_jog_joint(LR, LRC, "LRC");
    } else if (ps2x.Button(PSB_PAD_RIGHT)) {
      ps2_jog_joint(LR, LRF, "LRF");
    } else if (ps2x.Button(PSB_PAD_DOWN)) {
      ps2_jog_joint(LR, LRT, "LRT");
    } else if (ps2x.Button(PSB_PAD_LEFT)) {
      if (!activeServo[RRF]) {
        move_steps = ps2_jog_stick_steps();
        ps2_jog_all_legs(JOINT_FEMUR);
      }
    }
  }
}


//nudge ONE joint by a single pwm tick per poll: TRIANGLE raises, CROSS lowers.
//`label` is only used for the debug1 trace, so you can see which joint moved.
void ps2_jog_joint(int leg, int servo, const char* label) {
  if (activeServo[servo]) return;   //still finishing the last nudge

  int ms = servoPos[servo];
  if (ps2x.Button(PSB_TRIANGLE)) {
    ms += 1;
  } else if (ps2x.Button(PSB_CROSS)) {
    ms -= 1;
  }
  update_sequencer(leg, servo, spd, ms, 0, 0);

  if (debug1) {
    Serial.print(label); Serial.print(F(": "));
    Serial.println(limit_target(servo, ms, 0));
  }
}


//how far the right stick is asking a whole-body jog to travel
float ps2_jog_stick_steps() {
  return map(ps2x.Analog(PSS_RY), 0, 255, move_c_steps[0] / 1.5, move_c_steps[1] / 1.5);
}


//move the SAME joint on all four legs to home +/- move_steps.
//`joint` indexes servoLeg[]: JOINT_COXA, JOINT_FEMUR or JOINT_TIBIA.
void ps2_jog_all_legs(int joint) {
  for (int leg = 0; leg < TOTAL_LEGS; leg++) {
    int servo = servoLeg[leg][joint];
    update_sequencer(leg, servo, spd, (servoHome[servo] + move_steps), 0, 0);
  }
}


//runs while servo output is still disabled at boot.
//See init_pwm_and_servos() for why the PWM driver is held off until the
//PS2 link is up; this is the other half of that workaround.
void ps2_wait_for_connect() {
  //bench mode: with ps2_debug set before flashing, poll and report the
  //controller but leave OE disabled, so the remote can be verified without
  //the servos energising. clearing ps2_debug (serial 'ps2') falls through
  //to the normal wake below. ps2_check() stamps lastPS2Update on the way out.
  if (ps2_debug) {
    ps2x.read_gamepad(false, false);
    ps2_debug_report();
    return;
  }

  //draining ten reads settles the receiver before the PWM driver is allowed
  //to switch on - without this the servos twitch to whatever garbage the
  //first reads contain
  for (int i = 0; i < 10; i++) {
    ps2x.read_gamepad(false, false);
    delay(100);
  }

  if (rgb_active) {
    rgb_request(RGB_COUNT_3 RGB_SPEED_250 RGB_LEFT RGB_YELLOW RGB_RIGHT RGB_YELLOW RGB_PATTERN_BLINK);
  }

  //OE is ACTIVE LOW: driving it low ENABLES the servo outputs. From here on
  //Nova can move, so everything above must have finished settling.
  digitalWrite(OE_PIN, LOW);
  pwm_oe = 1;
}
