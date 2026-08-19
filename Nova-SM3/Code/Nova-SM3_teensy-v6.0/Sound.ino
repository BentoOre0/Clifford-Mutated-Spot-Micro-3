/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Teensy 4.0 (master)
 *
 *   Sound
 *
 *   Two independent sound paths: procedural buzzer tones on the piezo, and
 *   recorded tracks on the DFPlayer Mini. melody_active / buzz_active /
 *   mp3_active in NovaConfig.h decide which one a given event uses.
 *
 *   One tab of the Nova-SM3_teensy-v6.0 sketch. The Arduino IDE compiles
 *   every tab together as a single program, so the globals declared in
 *   Nova-SM3_teensy-v6.0.ino are visible here.
*/

/*
   -------------------------------------------------------
   TONES 
   -------------------------------------------------------
*/
void play_phrases() {
    int K = 2000;
    switch (random(1,9)) {
        case 1:phrase1(); phrase2(); break;
        case 2:phrase2(); phrase1(); break;
        case 3:phrase1(); phrase2(); phrase1(); break;
        case 4:phrase2(); phrase1(); phrase2(); break;
        case 5:phrase1(); phrase2(); phrase1(); phrase2(); break;
        case 6:phrase2(); phrase1(); phrase2(); phrase1(); break;
        case 7:phrase1(); phrase2(); phrase1(); phrase2(); phrase1(); break;
        case 8:phrase2(); phrase1(); phrase2(); phrase1(); phrase2(); break;
    }
    for (int i = 0; i <= random(3, 12); i++){
        tone(BUZZ, K + random(-1800, 2000));          
        delay(random(70, 170));  
        noTone(BUZZ);         
        delay(random(0, 30));             
    } 
    noTone(BUZZ);  
}


void phrase1() {    
    int k = random(1000,2000);
    for (int i = 0; i <=  random(100,2000); i++){
        tone(BUZZ, k+(-i*2));          
        delay(random(.9,2));             
    } 
    for (int i = 0; i <= random(100,1000); i++){
        tone(BUZZ, k + (i * 10));          
        delay(random(.9,2));             
    } 
}


void phrase2() {
    int k = random(1000,2000);
    for (int i = 0; i <= random(100,2000); i++){
        tone(BUZZ, k+(i*2));          
        delay(random(.9,2));             
    } 
    for (int i = 0; i <= random(100,1000); i++){
        tone(BUZZ, k + (-i * 10));          
        delay(random(.9,2));             
    } 
}


void melody1() {
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5;
    }

    tone(BUZZ, melody[thisNote], noteDuration * 0.9);

    delay(noteDuration);

    noTone(BUZZ);
  }
}


void mp3_play(int track) {

//check if currently playing, else wait

//    Serial.print("playing sound: ");Serial.print(track);
//    Serial.print("  state: ");Serial.print(DFPlayer.readState());
    DFPlayer.playMp3Folder(track);
//    Serial.print("  / ");Serial.println(DFPlayer.readState());
}


void mp3_volume(int vol) {
  if (vol < 0) {
//DEV: pot not working on SW panel :(
/*
    int svol = analogRead(MP3_VOL_PIN);
    if (svol < 1023) {
      sound_vol = map(analogRead(MP3_VOL_PIN), 0, 1023, 0, 30);  
    } else { 
      sound_vol = 12;
    }
*/
sound_vol = 12;
  } else {
    sound_vol = vol;
    DFPlayer.volume(sound_vol);
  }
}


void check_mp3() {
  int value;
  value = DFPlayer.readState();
    
  if (value == 2 || value == 0 || DFPlayer.available()) {
    play_mp3_queue(DFPlayer.readType());
  }

  lastMP3Update = millis();
}


void add_to_mp3_queue(int track) {
  for (int i=0;i<5;i++) {
    if (!mp3_queue[i]) {
      mp3_queue[i] = track;
      Serial.print(F("queued "));
      Serial.println(track);
      break;
    }
  }
}


void play_mp3_queue(uint8_t type) {
//  Serial.print(F("type "));
//  Serial.println(type);
  if (type == DFPlayerPlayFinished || type == 11) {
    for (int i=0;i<5;i++) {
      if (mp3_queue[i]) {
        mp3_play(mp3_queue[i]);
        Serial.print(F("play track "));Serial.println(mp3_queue[i]);
        mp3_queue[i] = 0;
        break;
      }
      if (i == 4) {
        mp3_status = 0;
      }
    }
  }
}
