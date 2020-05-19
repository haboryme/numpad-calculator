#ifndef CONFIG_H
#define CONFIG_H

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID       0xFEED
#define PRODUCT_ID      0x6060
#define DEVICE_VER      0x0001
#define MANUFACTURER    ms
#define PRODUCT         ms
#define DESCRIPTION     Numpad with basic oled calculator

/* key matrix size */
#define MATRIX_ROWS 6
#define MATRIX_COLS 4

/* key matrix pins */
#define MATRIX_ROW_PINS { F4, F5, F6, F7, B3, B2 }
#define MATRIX_COL_PINS { D4, C6, D7, E6 }
#define UNUSED_PINS

#define ENCODERS_PAD_A { D3 }
#define ENCODERS_PAD_B { D2 }

#define ENCODER_RESOLUTION 4

/* COL2ROW or ROW2COL */
#define DIODE_DIRECTION COL2ROW

/* number of backlight levels */
#define BACKLIGHT_PIN B5
#ifdef BACKLIGHT_PIN
#define BACKLIGHT_LEVELS 3
#endif

/* Set 0 if debouncing isn't needed */
#define DEBOUNCING_DELAY 5

/* Mechanical locking support. Use KC_LCAP, KC_LNUM or KC_LSCR instead in keymap */
#define LOCKING_SUPPORT_ENABLE

/* Locking resynchronize hack */
#define LOCKING_RESYNC_ENABLE

/* Oled */
#define OLED_DISPLAY_128X64
#define OLED_FONT_H "keyboards/zz15z3oled/glcdfont.c"
#define OLED_FONT_START 0
#define OLED_FONT_END 223

/* Tape dance */
//#define TAPPING_TERM 175

/* OPTI */
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT

#endif