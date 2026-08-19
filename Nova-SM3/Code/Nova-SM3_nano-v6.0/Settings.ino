/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Arduino Nano (slave)
 *
 *   Persistent settings and self-reset
 *
 *   One tab of the Nova-SM3_nano-v6.0 sketch.
*/


/*
   -------------------------------------------------------
   Load Settings
    :read the saved settings back over the defaults at the top of the sketch

    Only splash_active is ever written, by the reboot commands. The others are
    read but never saved, so on a board whose EEPROM has never been written
    they come back as 0xFF - which reads as true, and happens to match the
    intended defaults. Writing them would need a matching save command in the
    protocol, which does not exist yet.
   -------------------------------------------------------
*/
void load_ep_data() {
  debug1        = EEPROM.read(EEPROM_DEBUG1);
  rgb_active    = EEPROM.read(EEPROM_RGB_ACTIVE);
  oled_active   = EEPROM.read(EEPROM_OLED_ACTIVE);
  uss_active    = EEPROM.read(EEPROM_USS_ACTIVE);
  splash_active = EEPROM.read(EEPROM_SPLASH_ACTIVE);
}



/*
   -------------------------------------------------------
   Reset Slave
    :reboot this board on command from the master

    Pin 3 is wired back to this board's own RESET line, so pulling it LOW
    restarts the sketch. This call never returns.

    setup() deliberately drives the pin HIGH before switching it to an
    output, so that configuring it does not reset the board immediately.
   -------------------------------------------------------
*/
void reset_slave() {
  digitalWrite(RESET_PIN, LOW);
}
