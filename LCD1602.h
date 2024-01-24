#pragma once

#include <Arduino.h>

#define COLS  16
#define ROWS  2

#define TAB_WIDTH 4

// pins
#define RS_PIN  14
#define E_PIN   15
#define D4_PIN  33
#define D5_PIN  11
#define D6_PIN  31
#define D7_PIN  30
#define PWM_PIN 34

void lcd_begin();
void lcd_clear();
void lcd_home();
void lcd_setCursor(uint8_t col, uint8_t row);
void lcd_noDisplay();
void lcd_display();
void lcd_noCursor();
void lcd_cursor();
void lcd_noBlink();
void lcd_blink();
void lcd_scrollDisplayLeft();
void lcd_scrollDisplayRight();
void lcd_leftToRight();
void lcd_rightToLeft();
void lcd_autoscroll();
void lcd_noAutoscroll();
void lcd_createChar(uint8_t location, const uint8_t *charmap);

void lcd_beginUpdate();
void lcd_endUpdate();
bool lcd_updating();
void lcd_printc(char c);
void lcd_prints(const char *s);
