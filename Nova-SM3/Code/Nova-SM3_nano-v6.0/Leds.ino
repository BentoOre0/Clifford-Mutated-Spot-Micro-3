/*
 *   NovaSM3 - a Spot-Mini Micro clone   |   Version 6.0
 *   TARGET BOARD: Arduino Nano (slave)
 *
 *   NeoPixel eyes
 *
 *   Four pixels: 0 and 1 are the left eye, 2 and 3 the right. The master
 *   selects a pattern, a colour per side, a step interval and a repeat count
 *   (see the RGB_* constants in NovaSlaveProtocol.h); everything here just
 *   plays whatever was selected.
 *
 *   Most patterns are non-blocking: rgb_check() is called from loop() on the
 *   rgbInterval timer and each call advances the animation by ONE step, using
 *   pattern_step / current_led / fadeStep to remember where it was. The two
 *   exceptions are rainbow() and rgb_flow(), which run to completion inside a
 *   single call and will stall the rest of the board while they do.
 *
 *   One tab of the Nova-SM3_nano-v6.0 sketch.
*/


/*
   -------------------------------------------------------
   RGB Check
    :advance the eye animation by one step

    `pat` is the pattern number the master selected. The numbers match the
    RGB_* pattern constants in NovaSlaveProtocol.h, in the order listed there.

    Patterns 8, 11 and 12 count themselves down with pattern_cnt and hand
    control back to the idle wipe when they run out; the others repeat until
    the master selects something else.
   -------------------------------------------------------
*/
void rgb_check(int pat) {
  //brightness follows the panel potentiometer, backwards: turning it up
  //lowers the analog reading
  int bright = analogRead(LED_BRIGHT_PIN);
  rgb_bright = map(bright, 0, 1023, 100, 0);
  led_eyes.setBrightness(rgb_bright);
  run_pattern(pat);
}

void run_pattern(int pat) {
  switch (pat) {
    case 0:
      colorWipe(led_eyes.Color(0, 0, 0)); // off
      break;
    case 1:
      fade(0, 0, 0, 0, 0, 125, fade_steps, 1); // fade from blue to black
      break;
    case 2:
      fade(0, 255, 0, 64, 0, 0, fade_steps, 1); // fade from black to orange and back
      break;
    case 3:
      fade(0, 0, 0, 125, 0, 0, fade_steps, 1); // fade from green to black
      break;
    case 4:
      fade(0, 125, 0, 125, 0, 0, fade_steps, 1); // fade from yellow to black
      break;
    case 5:
      fade(0, 125, 0, 0, 0, 0, fade_steps, 1); // fade from red to black
      break;
    case 6:
      fade(0, 125, 0, 0, 0, 125, fade_steps, 1); // fade from purple to black
      break;
    case 7:
      colorWipe(led_eyes.Color(255, 255, 255)); //white
      break;
    case 8:
      if (pattern_cnt) {
        rgbInterval = pattern_int;
        colorWipePaired(led_eyes.Color(0, 32, 0)); // green wipe paired
      } else if (rgbInterval == pattern_int) {
        rgbInterval = 30;
        wipe_eyes();
      }
      break;
    case 9:
      rgb_flow(1000);
      break;
    case 10:
      rainbow(1000);
      break;
    case 11:
      if (pattern_cnt) {
        rgbInterval = pattern_int;
        blink_rgb(cur_rgb_val1, cur_rgb_val2);
      } else if (rgbInterval == pattern_int) {
        rgbInterval = 30;
        wipe_eyes();
      }
      break;
    case 12:
      if (pattern_cnt) {
        rgbInterval = pattern_int;
        colorWipeWave(led_eyes.Color(0, 32, 0)); // green wipe wave
      } else if (rgbInterval == pattern_int) {
        rgbInterval = 30;
        wipe_eyes();
      }
      break;
  }
}



void fade(int redStartValue, int redEndValue, int greenStartValue, int greenEndValue, int blueStartValue, int blueEndValue, int totalSteps, int fadeBack) {
  static float redIncrement, greenIncrement, blueIncrement;
  static float red, green, blue;
  static boolean fadeUp = false;

  if (fadeStep == 0) { // first step is to initialise the initial colour and increments
    red = redStartValue;
    green = greenStartValue;
    blue = blueStartValue;
    fadeUp = false;

    redIncrement = (float)(redEndValue - redStartValue) / (float)totalSteps;
    greenIncrement = (float)(greenEndValue - greenStartValue) / (float)totalSteps;
    blueIncrement = (float)(blueEndValue - blueStartValue) / (float)totalSteps;
    fadeStep = 1; // next time the function is called start the fade
  } else { // all other steps make a new colour and display it
    // make new colour
    red += redIncrement;
    green +=  greenIncrement;
    blue += blueIncrement;

    // set up the pixel buffer
    for (int i = 0; i < led_eyes.numPixels(); i++) {
      led_eyes.setPixelColor(i, led_eyes.Color((int)red, (int)green, (int)blue));
    }

    // now display it
    led_eyes.show();
    fadeStep += 1; // go on to next step
    if (fadeStep >= totalSteps) { // finished fade
      if (fadeUp) { // finished fade up and back
        fadeStep = 0;
        return; // so next call recalabrates the increments
      }
      // now fade back
      if (fadeBack) {
        fadeUp = true;
        redIncrement = -redIncrement;
        greenIncrement = -greenIncrement;
        blueIncrement = -blueIncrement;
        fadeStep = 1; // don't calculate the increments again but start at first change
      }
    }
  }
}

void colorWipe(uint32_t c) {
  static int i = 0;
  static int i2 = 1;
  wipe_eyes();
  led_eyes.setPixelColor(i, c);
  led_eyes.setPixelColor(i2, c);
  led_eyes.show();
  i++;
  i2++;
  if (i >= led_eyes.numPixels()) {
    i = 0;
    wipe_eyes(); // blank out led_eyes
  }
  if (i2 >= led_eyes.numPixels()) {
    i2 = 0;
    wipe_eyes(); // blank out led_eyes
  }
  lastRGBUpdate = millis();
}

void colorWipePaired(uint32_t c) {
  wipe_eyes();
  led_eyes.setPixelColor(current_led, c);
  if (current_led == 0) {
    led_eyes.setPixelColor(3, c);
    current_led++;
  } else {
    led_eyes.setPixelColor(2, c);
    current_led--;
  }
  led_eyes.show();
  lastRGBUpdate = millis();
}


void colorWipeWave(uint32_t c) {
  if (!current_led) {
    wipe_eyes();
  } else {  
    if (pattern_cnt) pattern_cnt--;
    led_eyes.setPixelColor((current_led-1), c);
    led_eyes.show();
  }

  if (current_led == led_eyes.numPixels()) {
    current_led = 0;
  } else {
    current_led++;
  }

  lastRGBUpdate = millis();
}

void rainbow(int wait) {
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < led_eyes.numPixels(); i++) {
      int pixelHue = firstPixelHue + (i * 65536L / led_eyes.numPixels());
      led_eyes.setPixelColor(i, led_eyes.gamma32(led_eyes.ColorHSV(pixelHue)));
    }
    led_eyes.show(); // Update strip with new contents
    delayMicroseconds(wait);
  }
  lastRGBUpdate = millis();
}

void rgb_flow(int wait) {
  unsigned int rgbColour[3];

  rgbColour[0] = 255;
  rgbColour[1] = 255;
  rgbColour[2] = 255;

  for (int decColour = 0; decColour < 3; decColour += 1) {
    int incColour = decColour == 2 ? 0 : decColour + 1;
    for (int i = 0; i < 255; i += 5) {
      rgb_del -= 5;
      if (rgb_del < 1) rgb_del = 1;
      rgbColour[decColour] -= 5;
      rgbColour[incColour] += 5;

      for (int ledcnt = 0; ledcnt < 4; ledcnt += 1) {
        led_eyes.setPixelColor(ledcnt, rgbColour[0], rgbColour[1], rgbColour[2]);
        led_eyes.show();
        delayMicroseconds(wait);
      }
    }
  }

  lastRGBUpdate = millis();
}

void wipe_eyes() { // clear all LEDs
  for (int i = 0; i < led_eyes.numPixels(); i++) {
    led_eyes.setPixelColor(i, led_eyes.Color(0, 0, 0));
  }
  led_eyes.show();
}

void blink_rgb(int rgb_val[3], int rgb_val2[3]) {
  if (pattern_cnt) {
    if (!pattern_step) {
      wipe_eyes();
      if (pattern_cnt) {
        pattern_step = 1;
      }
    } else {
      if (rgb_val2[0] || rgb_val2[1] || rgb_val2[2]) {
        for (int i = 0; i < led_eyes.numPixels(); i++) {
          if (i == 0 || i == 1) { 
            led_eyes.setPixelColor(i, led_eyes.Color(rgb_val[0], rgb_val[1], rgb_val[2]));
          } else {
            led_eyes.setPixelColor(i, led_eyes.Color(rgb_val2[0], rgb_val2[1], rgb_val2[2]));
          }
        }
      } else {
        for (int i = 0; i < led_eyes.numPixels(); i++) {
          led_eyes.setPixelColor(i, led_eyes.Color(rgb_val[0], rgb_val[1], rgb_val[2]));
        }
      }
      led_eyes.show();
      pattern_step = 0;
      pattern_cnt--;
    }
  } else {
    wipe_eyes();
  }

  lastRGBUpdate = millis();
}
