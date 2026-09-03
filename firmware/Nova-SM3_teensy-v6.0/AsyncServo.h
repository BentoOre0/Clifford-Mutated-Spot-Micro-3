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
 *   NOTE ON THE DESIGN
 *   While this is a "class", it does not follow the rules of writing classes.
 *   It holds almost no state of its own: position, speed, target, ramping and
 *   sweep parameters all live in the global arrays declared in NovaServos.h,
 *   and it calls the external functions set_ramp() and amperage_check().
 *
 *   That is deliberate for now - the movement routines all work by writing
 *   those arrays directly, so making the state private would mean rewriting
 *   every gait at the same time. The original author flagged this as
 *   something to fix before the class could be reused in another project.
*/


/*
   -------------------------------------------------------
   AsyncServo Class
    :non-blocking control of one PWM servo

    Nothing here ever waits. Update() is called once per servo on every pass
    of loop(), looks at the clock, and moves the servo AT MOST ONE pwm tick.
    A movement that takes a second is simply a few hundred of these calls.

    A servo is in one of two modes, and Update() handles them in this order:

      MOVE   activeServo[id] is set. Step towards targetPos[id], then clear
             activeServo[id] once it arrives.
      SWEEP  activeSweep[id] is set. Bounce between servoSweep[id][SWEEP_FROM]
             and [SWEEP_TO] for SWEEP_LOOPS passes. Only runs when the servo
             is not already doing a MOVE.

    Either mode may have a RAMP attached (servoRamp[id], set up by set_ramp()
    and enabled by the global use_ramp), which accelerates the servo away from
    the start of the travel and decelerates it into the end.

    servoSpeed[id] is the delay in milliseconds between ticks, so a BIGGER
    number is a SLOWER servo. That inversion trips people up constantly - it
    is why max_spd is numerically smaller than min_spd in NovaConfig.h.
   -------------------------------------------------------
*/
class AsyncServo {
    //properties of servo object
    Adafruit_PWMServoDriver *driver;    //reference to driver instantiation
    int servoID;                        //config ID of servo, indexes every array in NovaServos.h
    int pwmPin;                         //channel on the PCA9685 this servo is wired to
    int incUnit;                        //pwm ticks moved per step - 1, for the smoothest motion
    unsigned long lastUpdate;           //when this servo last stepped, for its speed timer


    //method to instantiate servo object
    public: AsyncServo(Adafruit_PWMServoDriver *Driver, int ServoId) {
      driver = Driver;                    //referenced copy of driver instantiation
      servoID = ServoId;                  //config ID of servo
      pwmPin = servoSetup[servoID][1];    //PCA9685 channel, NOT the same as servoID
      incUnit = 1;                        //pulse increment for each pulse movement of servo
    }


    /*
       Called once per servo, every pass of loop(). Returns immediately unless
       this servo is active AND its own speed interval has elapsed.
    */
    void Update() {
      updateMove();
      updateSweep();
    }


  private:

    /*
       Advance an ordinary move one tick towards targetPos.
    */
    void updateMove() {
      if (!activeServo[servoID]) return;

      //if servo is at destination, set to inactive
      if (servoPos[servoID] == targetPos[servoID]) {
        activeServo[servoID] = 0;
      }

      //not due to step yet
      if ((millis() - lastUpdate) <= servoSpeed[servoID]) return;
      lastUpdate = millis();

      interpolateRamp();
      traceStep();

      //a pending delay eats this step instead of moving
      if (servoDelay[servoID][DELAY_TICKS]) {
        servoDelay[servoID][DELAY_TICKS]--;
        return;
      }

      clampTargetToLimits();
      stepTowardsTarget();

      //count step
      servoStep[servoID]++;

      //check for end of step(s), where current position equals target position
      if (servoPos[servoID] == targetPos[servoID]) {
        arriveAtTarget();
      }
    }


    /*
       Advance a sweep one tick, reversing at each end until the loop count
       runs out. Skipped entirely while an ordinary move is in progress.
    */
    void updateSweep() {
      if (!activeSweep[servoID] || activeServo[servoID]) return;

      //if servo is done sweeping, set to inactive
      if (servoPos[servoID] == targetPos[servoID] && !servoSweep[servoID][SWEEP_LOOPS]) {
        activeSweep[servoID] = 0;
        //switch direction
        (servoSwitch[servoID]) ? servoSwitch[servoID] = 0 : servoSwitch[servoID] = 1;
      }

      //not due to step yet
      if ((millis() - lastUpdate) <= servoSpeed[servoID]) return;
      lastUpdate = millis();

      if (debug2 && servoID == debug_servo) {
        Serial.print(servoID); Serial.print(F("\ttarget: ")); Serial.print(targetPos[servoID]);
        Serial.print(F("\tstart: ")); Serial.print(servoSweep[servoID][SWEEP_FROM]);
        Serial.print(F("\ttarget: ")); Serial.print(servoSweep[servoID][SWEEP_TO]);
        Serial.print(F("\tdir: ")); Serial.print(servoSweep[servoID][SWEEP_DIR]);
        Serial.print(F("\tloops: ")); Serial.print(servoSweep[servoID][SWEEP_LOOPS]);
      }

      //a pending delay eats this step instead of moving
      if (servoSweep[servoID][SWEEP_DELAY]) {
        servoSweep[servoID][SWEEP_DELAY]--;
        if (debug2 && servoID == debug_servo) Serial.println(F(""));
        return;
      }

      interpolateRamp();
      traceStep();
      stepTowardsTarget();
      reverseSweepIfAtEnd();

      //count step
      servoStep[servoID]++;

      //check for end of sweep(s), where current position equals target position
      if (servoPos[servoID] == targetPos[servoID]) {
        arriveAtTarget();
      }

      if (debug2 && servoID == debug_servo) Serial.println(F(""));
    }


    /*
       Apply one tick of the acceleration / deceleration profile.

       The profile is a small state machine held entirely in servoRamp[], and
       each branch below is one of its states. It runs down RAMP_DISTANCE as
       the servo travels, speeding up over the first stretch and slowing down
       over the last. Identical for moves and sweeps, which is why it lives
       here rather than being written out twice.
    */
    void interpolateRamp() {
      if (!(servoRamp[servoID][RAMP_SPEED] && servoRamp[servoID][RAMP_DISTANCE])) return;

      if (servoRamp[servoID][RAMP_UP_SPEED] && servoRamp[servoID][RAMP_UP_DIST]) {
        //ramp up start: jump to the slow starting speed, once
        servoSpeed[servoID] = servoRamp[servoID][RAMP_UP_SPEED];
        servoRamp[servoID][RAMP_UP_SPEED] = 0;
        servoRamp[servoID][RAMP_UP_DIST]--;
        servoRamp[servoID][RAMP_DISTANCE]--;

      } else if (!servoRamp[servoID][RAMP_UP_SPEED] && servoRamp[servoID][RAMP_UP_DIST] && servoRamp[servoID][RAMP_UP_INC]) {
        //ramp up step: accelerate by shortening the inter-tick delay
        servoSpeed[servoID] -= servoRamp[servoID][RAMP_UP_INC];
        servoRamp[servoID][RAMP_UP_DIST]--;
        servoRamp[servoID][RAMP_DISTANCE]--;
        if (servoRamp[servoID][RAMP_UP_DIST] < 1) {  //ramp up end
          servoRamp[servoID][RAMP_UP_DIST] = 0;
          servoRamp[servoID][RAMP_UP_INC] = 0;
        }

      } else if (servoRamp[servoID][RAMP_DOWN_SPEED] && servoRamp[servoID][RAMP_DOWN_DIST] && servoRamp[servoID][RAMP_DISTANCE] <= servoRamp[servoID][RAMP_DOWN_DIST]) {
        //ramp down start: close enough to the end to begin slowing
        servoSpeed[servoID] += servoRamp[servoID][RAMP_DOWN_INC];
        servoRamp[servoID][RAMP_DOWN_SPEED] = 0;
        servoRamp[servoID][RAMP_DOWN_DIST]--;
        servoRamp[servoID][RAMP_DISTANCE]--;

      } else if (!servoRamp[servoID][RAMP_DOWN_SPEED] && servoRamp[servoID][RAMP_DOWN_DIST] && servoRamp[servoID][RAMP_DOWN_INC]) {
        //ramp down step: decelerate by lengthening the inter-tick delay
        servoSpeed[servoID] += servoRamp[servoID][RAMP_DOWN_INC];
        servoRamp[servoID][RAMP_DOWN_DIST]--;
        servoRamp[servoID][RAMP_DISTANCE]--;
        if (servoRamp[servoID][RAMP_DOWN_DIST] < 1) {  //ramp down end
          servoRamp[servoID][RAMP_DOWN_DIST] = 0;
          servoRamp[servoID][RAMP_DOWN_INC] = 0;
        }

      } else if (!servoRamp[servoID][RAMP_UP_SPEED] && !servoRamp[servoID][RAMP_UP_DIST] && !servoRamp[servoID][RAMP_UP_INC] && !servoRamp[servoID][RAMP_DOWN_SPEED] && !servoRamp[servoID][RAMP_DOWN_DIST] && !servoRamp[servoID][RAMP_DOWN_INC]) {
        //ramp clear: both phases finished, discard the profile
        for (int j = 1; j < 8; j++) {
          servoRamp[servoID][j] = 0;
        }

      } else if (servoRamp[servoID][RAMP_DOWN_DIST] && servoRamp[servoID][RAMP_DISTANCE]) {
        //cruising between the two ramps at a constant speed
        servoRamp[servoID][RAMP_DISTANCE]--;
      }
    }


    /*
       Keep targetPos inside this servo's calibrated travel.

       servoLimit pairs are ordered by leg direction rather than numerically,
       so which of the two is the upper bound depends on the leg - see the
       note beside servoLimit in NovaServos.h.
    */
    void clampTargetToLimits() {
      if (servoLimit[servoID][0] > servoLimit[servoID][1]) {
        //left leg: the pair reads high-to-low
        if (targetPos[servoID] > servoLimit[servoID][0]) {
          targetPos[servoID] = servoLimit[servoID][0];
        } else if (targetPos[servoID] < servoLimit[servoID][1]) {
          targetPos[servoID] = servoLimit[servoID][1];
        }
      } else {
        //right leg: the pair reads low-to-high
        if (targetPos[servoID] < servoLimit[servoID][0]) {
          targetPos[servoID] = servoLimit[servoID][0];
        } else if (targetPos[servoID] > servoLimit[servoID][1]) {
          targetPos[servoID] = servoLimit[servoID][1];
        }
      }
    }


    /*
       Move one increment towards targetPos and push it out to the driver.
    */
    void stepTowardsTarget() {
      if (servoPos[servoID] < targetPos[servoID]) {
        servoPos[servoID] += incUnit;
        driver->setPWM(pwmPin, 0, servoPos[servoID]);
      } else if (servoPos[servoID] > targetPos[servoID]) {
        servoPos[servoID] -= incUnit;
        driver->setPWM(pwmPin, 0, servoPos[servoID]);
      }
    }


    /*
       At the end of a sweep leg, turn around; at the end of a full pass,
       count one loop off the total.
    */
    void reverseSweepIfAtEnd() {
      if (servoPos[servoID] == targetPos[servoID] && !servoSweep[servoID][SWEEP_DIR]) {
        //reached SWEEP_TO, head back to SWEEP_FROM
        servoSweep[servoID][SWEEP_DIR] = 1;
        targetPos[servoID] = servoSweep[servoID][SWEEP_FROM];

        //reset ramp for next sweep
        if (use_ramp && servoRamp[servoID][RAMP_SPEED]) {
          servoSpeed[servoID] = servoRamp[servoID][RAMP_SPEED];
          set_ramp(servoID, servoSpeed[servoID], 0, 0, 0, 0);
        }
        if (debug2 && servoID == debug_servo) {
          Serial.print(F("\treversed"));
        }
      } else if (servoPos[servoID] == servoSweep[servoID][SWEEP_FROM] && servoSweep[servoID][SWEEP_DIR]) {
        //back at SWEEP_FROM, one out-and-back pass complete
        servoSweep[servoID][SWEEP_DIR] = 0;
        servoSweep[servoID][SWEEP_LOOPS]--;
        if (servoSweep[servoID][SWEEP_LOOPS]) {
          targetPos[servoID] = servoSweep[servoID][SWEEP_FROM];
          if (debug2 && servoID == debug_servo) {
            Serial.print(F("\tforward inc: ")); Serial.print(incUnit);
          }
        }
      }
    }


    /*
       Shared tidy-up for both modes once the servo reaches its target.
    */
    void arriveAtTarget() {
      //reset servo steps
      servoStep[servoID] = 0;

      //reset servo speed if done ramping
      if (use_ramp && servoRamp[servoID][RAMP_SPEED]) {
        servoSpeed[servoID] = servoRamp[servoID][RAMP_SPEED];
        servoRamp[servoID][RAMP_SPEED] = 0;
      }

      //IMPORTANT
      //this is the amp check that will catch jammed / over-extended motors.
      //A servo that has arrived but is still drawing heavily is stalled.
      if (amp_active) amperage_check(0);
    }


    /*
       Serial Plotter trace of one servo, enabled by debug2 in NovaConfig.h.
       Only the servo selected by debug_servo is traced, or the plot is
       unreadable with twelve of them writing at once.
    */
    void traceStep() {
      if (!(debug2 && servoID == debug_servo && plotter)) return;
      Serial.print(F("sPos:"));
      Serial.print(servoPos[servoID]);
      Serial.print(F("\tsSpd:"));
      Serial.println(servoSpeed[servoID]);
    }

};
