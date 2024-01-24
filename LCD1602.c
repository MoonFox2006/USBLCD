#include "LCD1602.h"

// commands
#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON   0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON    0x02
#define LCD_CURSOROFF   0x00
#define LCD_BLINKON     0x01
#define LCD_BLINKOFF    0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT   0x04
#define LCD_MOVELEFT    0x00

// flags for function set
#define LCD_8BITMODE  0x10
#define LCD_4BITMODE  0x00
#define LCD_2LINE     0x08
#define LCD_1LINE     0x00
#define LCD_5x10DOTS  0x04
#define LCD_5x8DOTS   0x00

static char _screen[ROWS][COLS];
static const uint8_t _row_offsets[4] = { 0, 0x40, COLS, 0x40 + COLS };
static uint8_t _displayfunction, _displaycontrol, _displaymode;
static uint8_t _col, _row;
static bool _updating;

static void pulseEnable() {
  digitalWrite(E_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(E_PIN, HIGH);
  delayMicroseconds(1); // enable pulse must be >450ns
  digitalWrite(E_PIN, LOW);
  delayMicroseconds(100); // commands need > 37us to settle
}

static void write4bits(uint8_t value) {
  digitalWrite(D4_PIN, value & 0x01);
  value >>= 1;
  digitalWrite(D5_PIN, value & 0x01);
  value >>= 1;
  digitalWrite(D6_PIN, value & 0x01);
  value >>= 1;
  digitalWrite(D7_PIN, value & 0x01);
  pulseEnable();
}

static void send(uint8_t value, uint8_t mode) {
  digitalWrite(RS_PIN, mode);
  write4bits(value >> 4);
  write4bits(value);
}

static inline void command(uint8_t value) {
  send(value, LOW);
}

static inline void write(uint8_t value) {
  send(value, HIGH);
}

static void scroll() {
  memmove(_screen[0], _screen[1], COLS * (ROWS - 1));
  memset(_screen[1], 0, COLS);
}

static void draw() {
  for (uint8_t y = 0; y < ROWS; ++y) {
    command(LCD_SETDDRAMADDR | _row_offsets[y]);
    for (uint8_t x = 0; x < COLS; ++x) {
      write(_screen[y][x] ? _screen[y][x] : ' ');
    }
  }
}

void lcd_begin() {
/*
  _row_offsets[0] = 0;
  _row_offsets[1] = 0x40;
  _row_offsets[2] = COLS;
  _row_offsets[3] = 0x40 + COLS;
*/
  _col = _row = 0;
  _updating = false;
  _displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
  pinMode(RS_PIN, OUTPUT);
  pinMode(E_PIN, OUTPUT);
  pinMode(D4_PIN, OUTPUT);
  pinMode(D5_PIN, OUTPUT);
  pinMode(D6_PIN, OUTPUT);
  pinMode(D7_PIN, OUTPUT);
  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
  delay(50);
  // Now we pull both RS and R/W low to begin commands
  digitalWrite(RS_PIN, LOW);
  digitalWrite(E_PIN, LOW);
  // put the LCD into 4 bit or 8 bit mode
  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03);
  delayMicroseconds(4500); // wait min 4.1ms
  // second try
  write4bits(0x03);
  delayMicroseconds(4500); // wait min 4.1ms
  // third go!
  write4bits(0x03); 
  delayMicroseconds(150);
  // finally, set to 4-bit interface
  write4bits(0x02); 
  // finally, set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);
  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  lcd_display();
  // clear it off
  lcd_clear();
  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);
}

void lcd_clear() {
  command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
  delayMicroseconds(2000); // this command takes a long time!
  memset(_screen, 0, sizeof(_screen));
  _col = _row = 0;
}

void lcd_home() {
  command(LCD_RETURNHOME); // set cursor position to zero
  delayMicroseconds(2000); // this command takes a long time!
  _col = _row = 0;
}

void lcd_setCursor(uint8_t col, uint8_t row) {
  if (col >= COLS)
    col = COLS - 1;
  if (row >= ROWS)
    row = ROWS - 1;
  command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
  _col = col;
  _row = row;
}

void lcd_noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_scrollDisplayLeft() {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void lcd_scrollDisplayRight() {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void lcd_leftToRight() {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

void lcd_rightToLeft() {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

void lcd_autoscroll() {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

void lcd_noAutoscroll() {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

void lcd_createChar(uint8_t location, const uint8_t *charmap) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (uint8_t i = 0; i < 8; ++i) {
    write(charmap[i]);
  }
}

inline void lcd_beginUpdate() {
  _updating = true;
}

void lcd_endUpdate() {
  if (_updating) {
    _updating = false;
    draw();
  }
}

inline bool lcd_updating() {
  return _updating;
}

void lcd_printc(char c) {
  if (c == '\b') {
    if (_col)
      --_col;
  } else if (c == '\r') {
    _col = 0;
  } else if (c == '\n') {
    if (_row < ROWS)
      ++_row;
  } else if (c == '\f') {
    memset(_screen, 0, sizeof(_screen));
    _col = _row = 0;
  } else if (c == '\t') {
    uint8_t spaces;

    if (_col >= COLS) {
      _col = 0;
      if (_row < ROWS)
        ++_row;
    }
    if (_row >= ROWS) {
      scroll();
      _row = ROWS - 1;
    }
    spaces = ((_col + TAB_WIDTH) / TAB_WIDTH) * TAB_WIDTH - _col;
    while ((_col < COLS) && spaces--) {
      _screen[_row][_col++] = ' ';
    }
  } else {
    if (_col >= COLS) {
      _col = 0;
      if (_row < ROWS)
        ++_row;
    }
    if (_row >= ROWS) {
      scroll();
      _row = ROWS - 1;
    }
    _screen[_row][_col++] = c;
  }
  if (! _updating)
    draw();
}

void lcd_prints(const char *s) {
  if (s && *s) {
    lcd_beginUpdate();
    while (*s) {
      lcd_printc(*s++);
    }
    lcd_endUpdate();
  }
}
