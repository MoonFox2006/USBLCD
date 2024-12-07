#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif

#include <stdlib.h>
//#include <include/ch5xx.h>
#include <Arduino.h>
#include "LCD1602.h"
#include "USBCDC.h"

#define USE_WDT

#define OFF_BRIGHT    0
#define NORMAL_BRIGHT 128
#define DIM_BRIGHT    64

#define DIM_TIME      10 // 10 sec. to DIM
#define OFF_TIME      5 // 5 sec. from DIM to OFF

#define ESC_CHAR          '\\'
#define ESC_CLEAR         ' '
#define ESC_HOME          '!'
#define ESC_OFF           '-'
#define ESC_ON            '+'
#define ESC_CURSOR_START  '['
#define ESC_CURSOR_END    ']'
#define ESC_BRIGHT_START  '{'
#define ESC_BRIGHT_END    '}'
#define ESC_PWM_START     '('
#define ESC_PWM_END       ')'
#define ESC_SAVE          '*'

enum { EEPROM_DIM_BRIGHT = 0, EEPROM_FULL_BRIGHT, EEPROM_DIM_TIME, EEPROM_OFF_TIME };

static char cmd[16];
static int8_t cmdLen = -1;
static bool enabled = true;
static uint8_t dimBright, fullBright;
static uint8_t dimTime, offTime;
static uint8_t bright;

#ifdef USE_WDT
static void wdtEnable() {
  WDOG_COUNT = 0;
  SAFE_MOD = 0x55;
  SAFE_MOD = 0xAA;
  GLOBAL_CFG |= bWDOG_EN;
}

static inline void wdtFeed() {
  WDOG_COUNT = 0;
}
#endif

static uint8_t atou(const char **s) {
  uint8_t result = 0;

  while ((**s >= '0') && (**s <= '9')) {
    result *= 10;
    result += *((*s)++) - '0';
  }
  return result;
}

static void load() {
  dimBright = eeprom_read_byte(EEPROM_DIM_BRIGHT);
  fullBright = eeprom_read_byte(EEPROM_FULL_BRIGHT);
  dimTime = eeprom_read_byte(EEPROM_DIM_TIME);
  offTime = eeprom_read_byte(EEPROM_OFF_TIME);
  if ((dimBright == 0xFF) && (fullBright == 0xFF) && (dimTime == 0xFF) && (offTime == 0xFF)) { // Empty EEPROM
    dimBright = DIM_BRIGHT;
    fullBright = NORMAL_BRIGHT;
    dimTime = DIM_TIME;
    offTime = OFF_TIME;
  }
}

static void save() {
  eeprom_write_byte(EEPROM_DIM_BRIGHT, dimBright);
  eeprom_write_byte(EEPROM_FULL_BRIGHT, fullBright);
  eeprom_write_byte(EEPROM_DIM_TIME, dimTime);
  eeprom_write_byte(EEPROM_OFF_TIME, offTime);
}

static void setBrightness(uint8_t pwm) {
  if (pwm == OFF_BRIGHT) { // Off
    pinMode(PWM_PIN, INPUT);
  } else {
    pinMode(PWM_PIN, OUTPUT);
    analogWrite(PWM_PIN, 255 - pwm);
  }
}

void setup() {
  USBInit();

  load();

  bright = fullBright;
  setBrightness((dimTime + offTime) > 0 ? bright : OFF_BRIGHT);

  lcd_begin();
  lcd_prints("CH552 LCD1602\r\nUSB display\r\n");

#ifdef USE_WDT
  wdtEnable();
#endif
}

void loop() {
  static uint32_t lastTime = 0;

#ifdef USE_WDT
  wdtFeed();
#endif

  if (USBSerial_available()) {
    lcd_beginUpdate();
    while (USBSerial_available()) {
      char c = USBSerial_read();

      if (cmdLen >= 0) { // ESC_CHAR already readed
        if (cmdLen == 0) { // First char after ESC_CHAR
          if (c == ESC_CLEAR) {
            lcd_printc('\f');
            cmdLen = -1;
          } else if (c == ESC_HOME) {
            lcd_setCursor(0, 0);
            cmdLen = -1;
          } else if (c == ESC_OFF) {
            lcd_noDisplay();
            setBrightness(OFF_BRIGHT);
            enabled = false;
            cmdLen = -1;
          } else if (c == ESC_ON) {
            lcd_display();
            setBrightness(bright);
            enabled = true;
            cmdLen = -1;
          } else if (c == ESC_SAVE) {
            save();
            cmdLen = -1;
          } else if ((c == ESC_CURSOR_START) || (c == ESC_BRIGHT_START) || (c == ESC_PWM_START)) {
            cmd[cmdLen++] = c;
          } else { // Not an escape!
            lcd_printc(ESC_CHAR);
            lcd_printc(c);
            cmdLen = -1;
          }
        } else { // "\\[c,r]" || "\\{d,o}" || "\\(d,f)"
          bool error = false;

          if ((c >= '0') && (c <= '9')) {
            if (cmdLen < sizeof(cmd) - 1)
              cmd[cmdLen++] = c;
            else
              error = true;
          } else if (c == ',') {
            if (! strchr(cmd, ','))
              cmd[cmdLen++] = c;
            else
              error = true;
          } else if (c == ESC_CURSOR_END) {
            error = true;
            if (cmd[0] == ESC_CURSOR_START) {
              char *s = &cmd[1];
              uint8_t c, r;

              c = atou(&s);
              if ((c >= 1) && (c <= COLS) && (*s == ',')) {
                ++s; // Skip ','
                r = atou(&s);
                if ((r >= 1) && (r <= ROWS) && (! *s)) {
                  lcd_setCursor(c - 1, r - 1);
                  cmdLen = -1;
                  error = false;
                }
              }
            }
          } else if (c == ESC_BRIGHT_END) {
            error = true;
            if (cmd[0] == ESC_BRIGHT_START) {
              char *s = &cmd[1];
              uint8_t d, o;

              d = atou(&s);
              if (*s == ',') {
                ++s; // Skip ','
                o = atou(&s);
                if (! *s) {
                  dimTime = d;
                  offTime = o;
                  cmdLen = -1;
                  error = false;
                }
              }
            }
          } else if (c == ESC_PWM_END) {
            error = true;
            if (cmd[0] == ESC_PWM_START) {
              char *s = &cmd[1];
              uint8_t d, f;

              d = atou(&s);
              if (*s == ',') {
                ++s; // Skip ','
                f = atou(&s);
                if (! *s) {
                  dimBright = d;
                  fullBright = f;
                  cmdLen = -1;
                  error = false;
                }
              }
            }
          } else // Wrong escape!
            error = true;
          if (error) {
            lcd_printc(ESC_CHAR);
            lcd_prints(cmd);
            lcd_printc(c);
            cmdLen = -1;
          }
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
  if (enabled) {
    uint8_t pwm;

    pwm = fullBright;
    if (dimTime + offTime > 0) {
      uint32_t time;

      time = millis() - lastTime;
      if ((offTime > 0) && (time > (dimTime + offTime) * 1000))
        pwm = OFF_BRIGHT;
      else if ((dimTime > 0) && (time > dimTime * 1000))
        pwm = dimBright;
    }
    if (bright != pwm) {
      bright = pwm;
      setBrightness(bright);
    }
  }
}
