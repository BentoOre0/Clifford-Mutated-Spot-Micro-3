/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Sensor polling
 *
 *   PIR motion, ultrasonic distance, the IMU, current draw and battery
 *   voltage. Each is polled from loop() on its own millis() interval and each
 *   can be switched off in NovaConfig.h.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   PIR Check
    :provide general description and explanation here
   -------------------------------------------------------
*/
void pir_check() {
  int mactive = mpu_active;
  int uactive = uss_active;

    if (move_follow) {
      follow();
    } else if (!pir_halt) {
      pir_val = digitalRead(PIR_FRONT);
      pir_wait--;
      if (pir_val == HIGH) {
        if (pir_state == LOW) {
          if (pir_wait < 1) {
            if (rgb_active) {
              rgb_request(RGB_SPEED_100 RGB_FADE_RED);
            }
            if (debug1)
              Serial.println(F("Motion detected!"));
    
            //disable mpu and uss sensors while in alert
            if (mpu_active) mpu_active = 0;
            if (uss_active) uss_active = 0;
    
            set_stop_active();
            pir_state = HIGH;
            for (int l = 0; l < TOTAL_LEGS; l++) {
              servoSequence[l] = 0;
            }
            for (int m = 0; m < TOTAL_SERVOS; m++) {
              if (is_left_leg(m)) {
                if (!is_front_leg(m) && (is_femur(m) || is_tibia(m))) {
                  servoStepMoves[m][0] = limit_target(m, (servoHome[m] - 30), 0);
                } else if (is_front_leg(m) && (is_femur(m) || is_tibia(m))) {
                  servoStepMoves[m][0] = limit_target(m, (servoHome[m] - 60), 0);
                } else if (is_front_leg(m)) {
                  servoStepMoves[m][0] = limit_target(m, (servoHome[m] - 20), 0);
                  servoStepMoves[m][1] = limit_target(m, (servoHome[m] + 35), 0);
                } else {
                  servoStepMoves[m][1] = limit_target(m, (servoHome[m] + 35), 0);
                }
              } else {
                if (!is_front_leg(m) && (is_femur(m) || is_tibia(m))) {
                  servoStepMoves[m][0] = limit_target(m, (servoHome[m] + 30), 0);
                } else if (is_front_leg(m) && (is_femur(m) || is_tibia(m))) {
                  servoStepMoves[m][0] = limit_target(m, (servoHome[m] + 60), 0);
                } else if (is_front_leg(m)) {
                  servoStepMoves[m][0] = limit_target(m, (servoHome[m] + 20), 0);
                  servoStepMoves[m][1] = limit_target(m, (servoHome[m] + 35), 0);
                } else {
                  servoStepMoves[m][1] = limit_target(m, (servoHome[m] + 35), 0);
                }
              }
              servoStepMoves[m][2] = servoHome[m];
            }
            spd_c = 1;
            spd_f = 1;
            spd_t = 1;
            move_loops = 0;
            move_delay = 300;
            move_sequence = 1;
    
            if (oled_active) {
              oled_request(OLED_INTRUDER);
            } else {
              if (debug1)
                Serial.println(F("INTRUDER ALERT!"));
            }
//DEV: add mp3_active  
            if (melody_active) {
              play_phrases();
            } else if (buzz_active) {
              for (int b = 3; b > 0; b--) {
                tone(BUZZ, 500);
                delay(70);
                noTone(BUZZ);
                delay(70);
              }
              noTone(BUZZ);
            }
    
            if (rgb_active) {
              rgb_request(RGB_SPEED_100 RGB_SOLID_WHITE);
            }
          }
        }
      } else {
        if (pir_state == HIGH) {
          Serial.println(F("Motion ended!"));
          pir_state = LOW;
          pir_wait = 100; //state machine cycles, not ms
          if (oled_active) {
            oled_request(OLED_ALERT_CLEAR);
          } else {
            if (debug1)
              Serial.println(F("ALERT COMPLETE!"));
          }
    
          pwm_oe = 0;
          pir_reset = 1;
        } else if (pir_reset) {
          if (!move_sequence) {
            pir_reset = 0;
            move_delays[0] = 3000;
            move_delay_sequences[0] = 7;
            move_delays[1] = 3000;
            move_delay_sequences[1] = 7;
            delay_sequences();
    
            if (rgb_active) {
              rgb_request(RGB_SPEED_2500 RGB_FADE_GREEN);
            }
          }
          if (oled_active) {
            oled_request(OLED_ALERT_CLEAR);
          }
    
          //re-enable mpu and uss sensors if enabled prior to alert
          mpu_active = mactive;
          uss_active = uactive;
        }
      }
    }

    lastPIRUpdate = millis();
}


void follow() {
  byte reset_scnt = 0;
  if (sound_cur == 0) {
    sound_cur = 25;
  }

  if (pir_wait) {
    pir_wait--;
  } else {
    int val_front = pir_frontState = digitalRead(PIR_FRONT);
    if (val_front == HIGH) {
      if (pir_frontState == LOW) {
        pir_frontState = HIGH;
      }
    } else {
      if (pir_frontState == HIGH) {
        pir_frontState = LOW;
      }
    }
  
    int val_left = pir_leftState = digitalRead(PIR_LEFT);
    if (val_left == HIGH) {            
      if (pir_leftState == LOW) {
        pir_leftState = HIGH;
      }
    } else {
      if (pir_leftState == HIGH) {
        pir_leftState = LOW;
      }
    }
  
    int val_right = pir_rightState = digitalRead(PIR_RIGHT);
    if (val_right == HIGH) {            
      if (pir_rightState == LOW) {
        pir_rightState = HIGH;
      }
    } else {
      if (pir_rightState == HIGH) {
        pir_rightState = LOW;
      }
    }

/*
    if (debug1) {
      Serial.print("F: ");Serial.print(pir_frontState);
      Serial.print("L: ");Serial.print(pir_leftState);
      Serial.print("R: ");Serial.println(pir_rightState);
    }
*/

    if (pir_frontState || pir_leftState || pir_rightState) {
      pir_wait = pirDelay;
      y_dir = 0;
      x_dir = 0;
      z_dir = 0;
    }

    if (move_wake) {
      set_stop();
      move_wake = 0;
    } else {
      if (!move_march && (pir_frontState || pir_leftState || pir_rightState) && !(pir_frontState && pir_leftState && pir_rightState)) {
        if (debug1)
          Serial.println("restart!");

        step_weight_factor = 1.20;
        move_steps = 25;
        if (mpu_is_active) mpu_active = 0;
        move_march = 1;

        pir_frontState = LOW;
        pir_leftState = LOW;
        pir_rightState = LOW;
      }  
  
      if (pir_frontState && !pir_leftState && !pir_rightState) {
        follow_dir = 1;
        if (debug1)
          Serial.println("go forward");
        y_dir = 15;
        if (sound_cur != 10) {
          sound_cur = 10;
          reset_scnt = 1;
        }
      } else if (pir_frontState && pir_leftState && !pir_rightState) {
        follow_dir = 2;
        if (debug1)
          Serial.println("go forward-left");
        y_dir = 10;
        x_dir = 10;
        if (sound_cur != 6) {
          sound_cur = 6;
          reset_scnt = 1;
        }
      } else if (pir_frontState && !pir_leftState && pir_rightState) {
        follow_dir = 3;
        if (debug1)
          Serial.println("go forward-right");
        y_dir = 10;
        x_dir = -10;
        if (sound_cur != 6) {
          sound_cur = 6;
          reset_scnt = 1;
        }
      } else if (!pir_frontState && pir_leftState && !pir_rightState) {
        follow_dir = 4;
        if (follow_dir_prev && follow_dir_prev != follow_dir) {
          if (debug1)
            Serial.println("detect look left");

          //stop, home, and save current spd / move_follow to restore after look_left
          set_stop_active();
          set_home();
          spd_prev = spd;
          spd = 1;
          set_speed();
          move_steps = 30;
          move_paused = "follow";
          move_look_left = 1;
        } else {
          if (debug1)
            Serial.println("go left");
          x_dir = -15;
        }
        if (sound_cur != 3) {
          sound_cur = 3;
          reset_scnt = 1;
        }
      } else if (!pir_frontState && !pir_leftState && pir_rightState) {
        follow_dir = 5;
        if (follow_dir_prev && follow_dir_prev != follow_dir) {
          if (debug1)
            Serial.println("detect look right");

          //stop, home, and save current spd / move_follow to restore after look_right
          set_stop_active();
          set_home();
          spd_prev = spd;
          spd = 1;
          set_speed();
          move_steps = 30;
          move_paused = "follow";          
          move_look_right = 1;
        } else {
          if (debug1)
            Serial.println("go right");
          x_dir = 15;
        }
        if (sound_cur != 3) {
          sound_cur = 3;
          reset_scnt = 1;
        }
      } else if (!pir_frontState && pir_leftState && pir_rightState) {
        follow_dir = 6;
        if (debug1)
          Serial.println("go back");
        y_dir = -15;
        if (sound_cur != 2) {
          sound_cur = 2;
          reset_scnt = 1;
        }
      } else if (pir_frontState && pir_leftState && pir_rightState) {
        follow_dir = 7;
        if (debug1)
          Serial.println("greet");
        move_loops = 2;
        move_switch = 2;
        move_wake = 1;
        move_march = 0;
      } else {
        follow_dir = 0;
        if (debug1)
          Serial.println("stop!");
        pir_repeat_cnt++;
        if (pir_repeat_cnt == 6) {
          set_stop();
          move_march = 0;
          pir_repeat_cnt = 0;
        }
        y_dir = 0;
        x_dir = 0;
        z_dir = 0;
        if (sound_cur != 25) {
          sound_cur = 25;
          reset_scnt = 1;
        }
      }

      if (mp3_active) {
        if (!sound_cnt || reset_scnt) {
          if (oled_active) {
            switch (follow_dir) {
              case 0: oled_request(OLED_RADAR_STOP); break;
              case 1: oled_request(OLED_RADAR_FORWARD); break;
              case 2: oled_request(OLED_RADAR_FWD_LEFT); break;
              case 3: oled_request(OLED_RADAR_FWD_RIGHT); break;
              case 4: oled_request(OLED_RADAR_LEFT); break;
              case 5: oled_request(OLED_RADAR_RIGHT); break;
              case 6: oled_request(OLED_RADAR_BACKWARD); break;
              case 7: oled_request(OLED_RADAR_GREET); break;
            }
          }
          mp3_play(6);
          reset_scnt = 0;
          sound_cnt = sound_cur;
        } else {
          sound_cnt--;
        }
      }

      follow_dir_prev = follow_dir;
    }
  }

}


/*
   -------------------------------------------------------
   Check Ultrasonic Reading
    :provide general description and explanation here
   -------------------------------------------------------
*/
void uss_check() {
  //check right sensor
  int dist_rt = command_slave(USS_READ_RIGHT);
  if (dist_rt != prev_distance_r && ((dist_rt > (prev_distance_r + distance_tolerance)) || (dist_rt < (prev_distance_r - distance_tolerance)))) {
    distance_r = prev_distance_r = dist_rt;
  }

  //check left sensor
  int dist_lt = command_slave(USS_READ_LEFT);
  if (dist_lt != prev_distance_l && ((dist_lt > (prev_distance_l + distance_tolerance)) || (dist_lt < (prev_distance_l - distance_tolerance)))) {
    distance_l = prev_distance_l = dist_lt;
  }

  if (oled_active) {
    oled_request(OLED_DISTANCES);
  }


/*
  if (dist_rt && dist_lt && (dist_rt < distance_alarm || dist_lt < distance_alarm)) {
    distance_alarm_set++;

    if (debug7 && distance_alarm_set > 0) {
      Serial.print(F("USS ALARM #"));Serial.println(distance_alarm_set);
    }
    Serial.print(F("USS ALARM #"));Serial.println(distance_alarm_set);

    if (oled_active && distance_alarm_set < 3) {
      oled_request(OLED_DISTANCES);
    } else {
      set_stop_active();
      if (oled_active) {
        oled_request(OLED_INTRUDER);
      }
      rgb_request(RGB_LEFT RGB_BLUE RGB_RIGHT RGB_RED RGB_SPEED_100 RGB_COUNT_25 RGB_PATTERN_BLINK);
      delay(3000);
      set_stop();
      distance_alarm_set = 0;
    }
    
    if (debug7) {
      if (plotter) {
        Serial.print(F("Lt:"));
        if (distance_l > (distance_alarm*5)) {
          Serial.print((distance_alarm*5));
        } else {
          Serial.print(distance_l);
        }
        Serial.print(F("\t"));
        Serial.print(F("Rt:"));
        if (distance_r > (distance_alarm*5)) {
          Serial.print((distance_alarm*5));
        } else {
          Serial.print(distance_r);
        }
        Serial.print(F("\t"));
        Serial.println();
      } else {
        Serial.print(F("Dist Right: "));
        Serial.println(distance_r);
        Serial.print(F("Dist Left: "));
        Serial.println(distance_l);
      }
    }
  } else {
    distance_alarm_set = 0;
  }
*/

  lastUSSUpdate = millis();
}


/*
   -------------------------------------------------------
   Get MPU Data
    :provide general description and explanation here
   -------------------------------------------------------
*/
void get_mpu() {

  if(readByte(MPU6050_ADDRESS, INT_STATUS) & 0x01) {  // check if data ready interrupt
    readAccelData(accelCount);  // Read the x/y/z adc values
    getAres();
    
    // calculate the accleration value into actual g's
//    ax = (float)accelCount[0]*aRes - accelBias[0];  // get actual g value, this depends on scale being set
//    ay = (float)accelCount[1]*aRes - accelBias[1];   
//    az = (float)accelCount[2]*aRes - accelBias[2];  
    ax = (float)accelCount[0] - SelfTest[0];
    ay = (float)accelCount[1] - SelfTest[1];   
    az = (float)accelCount[2] - SelfTest[2];  


    // Calculating Roll and Pitch from the accelerometer data
    accAngleX = (atan(ay / sqrt(pow(ax, 2) + pow(az, 2))) * 180 / PI);// - SelfTest[0]; // SelfTest[0] ~(0.58) See the calculate_IMU_error()custom function for more details
    accAngleY = (atan(-1 * ax / sqrt(pow(ay, 2) + pow(az, 2))) * 180 / PI);// - SelfTest[1]; // SelfTest[1] ~(-1.58)
   
    readGyroData(gyroCount);  // Read the x/y/z adc values
    getGres();
 
    // calculate the gyro value into actual degrees per second
    gx = (float)gyroCount[0]*gRes - gyroBias[0];  // get actual gyro value, this depends on scale being set
    gy = (float)gyroCount[1]*gRes - gyroBias[1];  
    gz = (float)gyroCount[2]*gRes - gyroBias[2];   
  
    // Correct the outputs with the calculated error values
//    gx = gx + abs(SelfTest[3]); // SelfTest[3] ~(-0.56)
//    gy = gy + abs(SelfTest[4]); // SelfTest[4] ~(2)
//    gz = gz + abs(SelfTest[5]); // SelfTest[5] ~ (-0.8)

    // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by seconds (s) to get the angle in degrees
    previousTime = currentTime;        // Previous time is stored before the actual time read
    currentTime = millis();            // Current time actual time read
    elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds

    gyroAngleX = gyroAngleX + gx * (elapsedTime/2); // deg/s * s = deg
    gyroAngleY = gyroAngleY + gy * (elapsedTime/2);
    myaw = (myaw + gz * (elapsedTime/2));

    // Complementary filter - combine acceleromter and gyro angle values
    mroll = (0.97 * gyroAngleX + 0.03 * accAngleX);
    mpitch = (0.97 * gyroAngleY + 0.03 * accAngleY);
  }  

  if (!plotter && debug5) {
//    Serial.print("mpu x / y / z:\t\t"); Serial.print(mroll); Serial.print("\t/\t"); Serial.print(mpitch); Serial.print("\t/\t"); Serial.println(myaw);
    Serial.print("mpu x / y:\t\t"); Serial.print(mroll); Serial.print("\t/\t"); Serial.println(mpitch);
  } else if (plotter && debug5) {
//    Serial.print("x:"); Serial.print(mroll); Serial.print("\ty:"); Serial.print(mpitch); Serial.print("\tz:"); Serial.println(myaw);
    Serial.print("roll:"); Serial.print(mroll); Serial.print("\tpitch:"); Serial.println(mpitch);
  }
    
  //on init mpu, save offsets as defaults for resetting MPU position
  if (mpuInterval != mpuInterval_prev){
    mpuInterval = mpuInterval_prev;
    mpu_mroll = mroll;
    mpu_mpitch = mpitch;
    mpu_myaw = myaw;

    //delay before starting set_axis first time
    delay(1000);
    if (!plotter && debug) {
      Serial.println(F("\nNova SM3... \t\t\t\tReady!"));
      Serial.println(F("=============================================="));
    
      delay(500);
      if (!plotter && serial_active) {
        Serial.println();
        Serial.println(F("Type a command input or 'h' for help:"));
      }
    }
  }

  if (!plotter) {
    set_axis(mroll, mpitch);
  }

  lastMPUUpdate = millis();
}


/*
   -------------------------------------------------------
   Check Amperage Level
    :provide general description and explanation here
   -------------------------------------------------------
*/
void amperage_check(int aloop) {
  float AcsValue=0.0,Samples=0.0,AvgAcs=0.0,AcsValueF=0.0;

  for (int x = 0; x < 150; x++){ //Get 150 samples
    AcsValue = analogRead(AMP_PIN);     //Read current sensor values   
    Samples = Samples + AcsValue;  //Add samples together
//state machine this if possible... adds blocking delay to code, all be it miniscule!!
    delay(3); // let ADC settle before next sample 3ms
  }
  AvgAcs=Samples/150.0;//Taking Average of Samples
  AcsValueF = (2.5 - (AvgAcs * (5.0 / 1024.0)) )/0.177;
  
  AcsValueF = abs(AcsValueF)*1.8;
  if (debug4) {
    Serial.print(AvgAcs);
    Serial.print("/");
    Serial.print(AcsValueF);
    Serial.print("/");
    Serial.print(amp_limit);
  }
  if (AcsValueF > amp_limit) {
    ampInterval = 10;
    if (amp_cnt < amp_thresh) {
      amp_cnt++;
      rgb_request(RGB_COUNT_2);
      if (debug4) {
        Serial.print("\tlimit met, counting... ");Serial.println(amp_cnt);
      }
    } else {
      rgb_request(RGB_COUNT_100);
      detach_all();
      if (debug4) {
        Serial.println(F("\tthresh met, shutdown pwm!"));
      }
    }

    if (rgb_active) {
      rgb_request(RGB_LEFT RGB_RED RGB_RIGHT RGB_YELLOW RGB_SPEED_30 RGB_PATTERN_BLINK);
    }
  } else if (AcsValueF > amp_limit/2) {
    if (amp_cnt < amp_thresh) {
      rgb_request(RGB_COUNT_2);
      amp_cnt++;
    } else {
      rgb_request(RGB_COUNT_100);
      detach_all();
      if (debug4) {
        Serial.println(F("\tsoft thresh met, shutdown!"));
      }
    }
    ampInterval = 50;
    amp_warning = 2;

    if (rgb_active) {
      rgb_request(RGB_LEFT RGB_ORANGE RGB_RIGHT RGB_YELLOW RGB_SPEED_30 RGB_PATTERN_BLINK);
    }
    if (debug4) {
      Serial.print("\twarning 2 set - cnt: ");Serial.println(amp_cnt);
    }
  } else if (AcsValueF > amp_limit/3) {
    if (amp_cnt) {
      amp_cnt--;
    }
    rgb_request(RGB_COUNT_1);
    ampInterval = 300;
    amp_warning = 1;

    if (rgb_active) {
      rgb_request(RGB_LEFT RGB_GREEN RGB_RIGHT RGB_YELLOW RGB_SPEED_30 RGB_PATTERN_BLINK);
    }
    if (debug4) {
      Serial.print("\twarning 1 set - cnt: ");Serial.println(amp_cnt);
    }
  } else {
    if (amp_cnt) {
      amp_cnt--;
    }
    ampInterval = 1000;
    if (debug4) {
      Serial.print("\tcnt: ");Serial.println(amp_cnt);
    }
  }

  if (!aloop) {
    ampInterval = 0;
  }
   lastAmpUpdate = millis();
}


/*
   -------------------------------------------------------
   Check Battery Level
    :provide general description and explanation here
   -------------------------------------------------------
*/
void battery_check() {
  int syshalt = 0;
  int batt_danger = 0;
  int sensorValue = analogRead(BATT_MONITOR);
  //scale the raw ADC reading back up through the fitted voltage divider
  batt_voltage = sensorValue * (BATT_ADC_REF / BATT_ADC_STEPS) * BATT_DIVIDER_RATIO;

  if (batt_voltage <= (batt_voltage_prev - .05)) {
    if (batt_cnt == 3) {
      batt_voltage = avg_volts / batt_cnt;
      for (int i = 0; i < 9; i++) {
        if (batt_voltage <= batt_levels[i]) {
          batt_danger++;
        } else if ((batt_voltage >= (batt_levels[i] - 0.02)) && (batt_voltage <= (batt_levels[i] + 0.02))) {
          batt_danger = i;
        }
      }
      if (debug4 || debug1) {
        Serial.print(batt_voltage); Serial.print(" volts - BATTERY LEVEL #"); Serial.println(batt_danger);
      }

//   11.2, 11.1, 11.0, 10.9, 10.8, 
//   10.7, 10.6, 10.5, 10.4, 10.3,
  
      if (oled_active) {
        switch ((batt_danger)) {
          case 0: oled_request(OLED_BATTERY_LEVEL_0 OLED_BATTERY_GAUGE); if (mp3_active) mp3_play(18); break;
          case 1: oled_request(OLED_BATTERY_LEVEL_1 OLED_BATTERY_GAUGE); break;
          case 2: oled_request(OLED_BATTERY_LEVEL_2 OLED_BATTERY_GAUGE); if (mp3_active) mp3_play(18); break;
          case 3: oled_request(OLED_BATTERY_LEVEL_3 OLED_BATTERY_GAUGE); break;
          case 4: oled_request(OLED_BATTERY_LEVEL_4 OLED_BATTERY_GAUGE); if (mp3_active) mp3_play(19); break;
          case 5: oled_request(OLED_BATTERY_LEVEL_5 OLED_BATTERY_GAUGE); break;
          case 6: oled_request(OLED_BATTERY_LEVEL_6 OLED_BATTERY_GAUGE); if (mp3_active) mp3_play(20); break;
          case 7: oled_request(OLED_BATTERY_LEVEL_7 OLED_BATTERY_GAUGE); break;
          case 8: oled_request(OLED_BATTERY_LEVEL_8 OLED_BATTERY_GAUGE); if (mp3_active) mp3_play(21); break;
          case 9: oled_request(OLED_BATTERY_LEVEL_9 OLED_BATTERY_GAUGE); syshalt = 1; break;
          default: oled_request(OLED_BATTERY_LEVEL_9 OLED_BATTERY_GAUGE); syshalt = 1; break;
        }
      }
      batt_voltage_prev = batt_voltage;
      batt_cnt = 0;
      avg_volts = 0; 
       
      if (syshalt) {
        if (debug4 || debug1) {
          Serial.print(batt_voltage); Serial.println(F(" volts - SYSTEM HALTED!"));
        }
        powering_down();
        while(1) {           //simulate system halt
          if (oled_active) {
            oled_request(OLED_BATTERY_LEVEL_9 OLED_BATTERY_GAUGE);
            delay(3000);
            oled_request(OLED_HALTED);
            delay(3000);
          }
        }
      }
    } else {
      batt_cnt++;
      avg_volts += batt_voltage;
      if (debug4) {
        Serial.print("batt change #"); Serial.print(batt_cnt); 
        Serial.print(": sensor / volts "); Serial.print(sensorValue); Serial.print(F(" / ")); Serial.println(batt_voltage);
      }
    }
  }

  lastBatteryUpdate = millis();
}
