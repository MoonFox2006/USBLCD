#include <stdlib.h>
#include <Arduino.h>
#include "LCD1602.h"

#define NORMAL_BRIGHT 128
#define HALF_BRIGHT   192

#define HALF_TIME     10000 // 10 sec. to HALF
#define OFF_TIME      15000 // 15 sec. to OFF

#define ESC_CHAR      '\\'
#define ESC_CLEAR     ' '
#define ESC_HOME      '!'
#define ESC_CURSOR    '='
#define ESC_OFF       '-'
#define ESC_ON        '+'

static char cmd[6];
static int8_t cmdLen = -1;
static uint8_t bright = NORMAL_BRIGHT;

static uint8_t atou(const char *s) {
  uint8_t result = 0;

  while ((*s >= '0') && (*s <= '9')) {
    result *= 10;
    result += *s++ - '0';
  }
  return result;
}

void setup() {
  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, bright);
  lcd_begin();
  lcd_prints("CH552 LCD1602\r\nUSB display\r\n");
}

void loop() {
  static uint32_t lastTime = 0;

  uint32_t time;
  uint8_t pwm;

  if (USBSerial_available()) {
    lcd_beginUpdate();
    while (USBSerial_available()) {
      char c = USBSerial_read();

      if (cmdLen >= 0) {
        if (cmdLen == 0) {
          if (c == ESC_CLEAR) {
            lcd_printc('\f');
            cmdLen = -1;
          } else if (c == ESC_HOME) {
            lcd_setCursor(0, 0);
            cmdLen = -1;
          } else if (c == ESC_OFF) {
            lcd_noDisplay();
            cmdLen = -1;
          } else if (c == ESC_ON) {
            lcd_display();
            cmdLen = -1;
          } else if (c == ESC_CURSOR) {
            cmd[cmdLen++] = c;
          } else { // Not an escape!
            lcd_printc(ESC_CHAR);
            lcd_printc(c);
            cmdLen = -1;
          }
        } else { // "\\=c,r"
//          if (cmd[0] == ESC_CURSOR) {
            if ((c >= '0') && (c <= '9')) {
              cmd[cmdLen++] = c;
              if ((cmdLen > 3) && (cmd[cmdLen - 2] == ',')) {
                lcd_setCursor(atou(&cmd[1]), c - '0');
                cmdLen = -1;
              } else if (cmdLen >= sizeof(cmd) - 1) { // Wrong escape!
                lcd_printc(ESC_CHAR);
                lcd_prints(cmd);
                cmdLen = -1;
              }
            } else if ((c == ',') && (cmdLen > 1) && (! strchr(cmd, ','))) {
              cmd[cmdLen++] = c;
            } else { // Wrong escape!
              lcd_printc(ESC_CHAR);
              lcd_prints(cmd);
              lcd_printc(c);
              cmdLen = -1;
            }
//          }
        }
      } else {
        if (c == ESC_CHAR) {
          memset(cmd, 0, sizeof(cmd));
          cmdLen = 0;
        } else
          lcd_printc(c);
      }
    }
    lcd_endUpdate();
    lastTime = millis();
  }
  time = millis() - lastTime;
  if (time > OFF_TIME)
    pwm = 255;
  else if (time > HALF_TIME)
    pwm = HALF_BRIGHT;
  else
    pwm = NORMAL_BRIGHT;
  if (bright != pwm) {
    bright = pwm;
    if (bright == 255) // Off
      pinMode(PWM_PIN, INPUT);
    else {
      pinMode(PWM_PIN, OUTPUT);
      analogWrite(PWM_PIN, bright);
    }
  }
}
