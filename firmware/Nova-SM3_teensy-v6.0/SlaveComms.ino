/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Talking to the Nano slave
 *
 *   The i2c conversation with the slave board. Command bytes are named in
 *   NovaSlaveProtocol.h - use rgb_request() and oled_request() rather than
 *   command_slave() directly, so the slave's mode flag is handled for you.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   Serial Communication Functions
   -------------------------------------------------------
*/
void rgb_request(const char* commands) {
  if (serial_oled) {
    serial_oled = command_slave(SLAVE_TOGGLE_MODE);
  }
  serial_resp = command_slave(commands);
}


void oled_request(const char* commands) {
  if (!serial_oled) {
    serial_oled = command_slave(SLAVE_TOGGLE_MODE);
  }
  serial_resp = command_slave(commands);
  serial_oled = command_slave(SLAVE_TOGGLE_MODE);
}


int command_slave(const char* commands) {
  int command_response = 0;

  if(commands && slave_active) {
    int i = 0;
    while(commands[i]) {
      if (debug6 && (commands[i] != 'Z')) {
        (serial_oled) ? Serial.print(F("OLED Command: ")) : Serial.print(F("System Command: "));
      }

      Wire1.beginTransmission((uint8_t)SLAVE_ID);
      Wire1.write(char(commands[i]));
      Wire1.endTransmission();
      if (debug6 && (commands[i] != 'Z')) {
        Serial.print(commands[i]);
      }
      
      Wire1.beginTransmission((uint8_t)SLAVE_ID);
      int available = Wire1.requestFrom((uint8_t)SLAVE_ID, (uint8_t)2);
              
      if (available == 2) {
        command_response = Wire1.read() << 8 | Wire1.read(); 
        if (debug6 && (commands[i] != 'Z')) {
          Serial.print("\tresponse: ");
          Serial.print(command_response);
        }
      }
      Wire1.endTransmission();

      if (debug6 && (commands[i] != 'Z')) {
        Serial.println();
      }

      //if resetting slave, pause 15secs for display graphics if splash_active
      if (commands[i] == 'Z' || commands[i] == 'Y') {
        if (debug) Serial.print(F("Slave Circuit intializing..."));
        int pcnt = 6;
        if (commands[i] == 'Z' && oled_active && splash_active) {
          pcnt = 14;
        }
        for(int n=0;n<pcnt;n++) {
          if (debug) Serial.print(F("."));
          delay(1000);
        }
        if (commands[i] == 'Z' && oled_active && splash_active) {
          if (debug) Serial.println(F("\tOK"));
        } else {
          if (debug) Serial.println(F("\t\tOK"));
        }
        delay(1000);
      }
      
      i++;
    }
  }

  return command_response;
}
