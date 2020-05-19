#include QMK_KEYBOARD_H
#include <string.h>
#include <stdlib.h>
#include "quantum.h"
#include <stdint.h>
#include <avr_f64.h>

/*
* -----------------------------------
* Here is the part for the keymap
* -----------------------------------
*/

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[0] = KEYMAP(
		KC_CALC,                          KC_MUTE, 
		LT(1, KC_NLCK), KC_PSLS, KC_PAST, KC_PMNS, 
		KC_P7,          KC_P8,   KC_P9,   KC_PPLS,
		KC_P4,          KC_P5,   KC_P6, 
		KC_P1,          KC_P2,   KC_P3,   KC_PENT,
		KC_P0,                   KC_PDOT),

    [1] = KEYMAP(
		KC_TRNS,                   KC_TRNS, 
		KC_TRNS, KC_TRNS, KC_TRNS, KC_VOLD, 
		KC_TRNS, BL_INC,  KC_TRNS, KC_VOLU,
		KC_TRNS, BL_TOGG, KC_TRNS, 
		KC_TRNS, BL_DEC,  KC_TRNS, RESET,
		KC_TRNS,          KC_TRNS),
};

void matrix_init_user(void) {
}

void matrix_scan_user(void) {
}

void led_set_user(uint8_t usb_led) {
	if (IS_LED_ON(usb_led, USB_LED_NUM_LOCK)) {
		writePinHigh(B6);
	} else {
		writePinLow(B6);
	}
}

void encoder_update_user(uint8_t index, bool clockwise) {
  if (index == 0) { /* First encoder */
    if (clockwise) {
      tap_code(KC_VOLU);
    } else {
      tap_code(KC_VOLD);
    }
  }
}

/*
* -----------------------------------
* Here is the part for the calculator
* -----------------------------------
*/

enum operators { enter, add, subtract, multiply, divide, empty };
int               has_dot             = false;  // Is there already a dot in big_output?
int               positive            = true;   // sign of big_output
int               current_index_big   = 0;      // current position in the big output + indicator for 0 division
unsigned int      current_index_small = 0;      // current position in the small output
const uint8_t     MAX_SMALL           = 21;
const uint8_t     MAX_BIG             = 11;
static const char reset_big[12]       = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
static const char reset_small[22]     = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
char              big_output[12]      = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
char              small_output[22]    = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
int               operator            = empty;
int               is_op               = false;  // flag to know if last action was an operator (excluding =)
char *nullptr;
float64_t small_value = 0;  // computed value in small area
float64_t big_value   = 0;  // value currently being displayed
struct RightValue {
    float64_t content; //value of RightValue
    char   str[12]; // string representation of the value
    int    validity; // whether or not it should be considered for condition check
};
struct RightValue right_value = {0, "", 0};

/*
 * Function:  get_length
 * --------------------
 * give the length of char array
 * only used for a particular case
 *  c: char array
 *
 *  returns: give the length of the array considering anything but ' ' as non empty
 *           stops at the first ' ' encountered going from left to right
 */
int get_length(char *c) {
    int len = strlen(c);
    for (int i = 0; i < len; i++) {
        if (c[i] == ' ') {
            return (i);
        }
    }
    return (len);
}


/* 
* Display section 
*/

char output[] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00
};


/*
 * Function:  set_small_output
 * --------------------
 * Write the value of small_output to output
 * Mapping between ascii table and glcfont.c is direct
 */
void set_small_output(void){
    for (int i=42; i<63; i++){
        output[i] = 0x20;
    }
    int end = get_length(small_output) - 1;
    for (int i=end; i>=0; i--){
        output[62 - (end - i)] = small_output[i];
    }
}

/*
 * Function:  add_2x3_char
 * --------------------
 * Add to a string a character built with 6 tiles defined in glcfont.c
 */
void add_2x3_char(char* out, char in, int* anchor, int offset, int shift){
    out[*anchor - 1 - offset] = in + shift;
    out[*anchor -1 + 21 - offset] = in + shift + 32;
    out[*anchor -1 + 42 - offset] = in + shift + 64;
    out[*anchor - offset] = in + shift + 1;
    out[*anchor + 21 - offset] = in + shift + 32 + 1;
    out[*anchor + 42 - offset] = in + shift + 64 + 1;
    --*anchor;
}

/*
 * Function:  set_big_output
 * --------------------
 * Write the value contained in big_output to the screen
 * Characters are 2x3 tiles
 */
void set_big_output(void) {
    for (int i=105; i<168; i++){
        output[i] = 0x20;
    }
    int end = get_length(big_output) - 1;
    int last = 125; // TODO SET WITH CONFIG
    for (int i = end; i >= 0; i--) {
        int offset = end - i;
        if (big_output[i] == '.'){
            output[last - offset] = big_output[i] + 102;
            output[last + 21 - offset] = big_output[i] + 102 + 32;
            output[last + 42 - offset] = big_output[i] + 102 + 64;
        }else if ((big_output[i] == '+') || (big_output[i] == '-')){
            int shift = 112;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
        }else if (big_output[i] == 'E'){
            int shift = 80;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
		}else if (big_output[i] == 'a'){
			int shift = 54;
			char c = big_output[i];
			add_2x3_char(output, c, &last, offset, shift);
		}else if (big_output[i] == 'N'){
            int shift = 75;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
        }else{
            int shift = 80 + big_output[i] - 48;
            char c = big_output[i];
            add_2x3_char(output, c, &last, offset, shift);
        }
    }
}

void render(void){
    set_small_output();
    set_big_output();
    oled_clear();
    oled_write(output, false);
}

/* 
* Handling section
*/

/*
 * Function:  reset
 * --------------------
 * Resets all variables
 */
void reset(void) {
    oled_clear();
    current_index_big   = 0;
    current_index_small = 0;
    memcpy(small_output, reset_small, sizeof(reset_small));
    memcpy(big_output, reset_big, sizeof(reset_big));
    has_dot              = false;
    operator             = empty;
    positive             = true;
    right_value.validity = 0;
    is_op                = false;
    return;
}


/*
 * Function:  offset_small_output
 * --------------------
 * Modifies small_output (global) to move characters to the left by a given amount
 * Offsets character that are not overwritten remain.
 *  int: offset, by how many unit small_output is offset to the left
 *
 * Example: 
 *  123456 (3) -> 456456
 */
void offset_small_output(int offset) {
    for (int i = 0; i < (MAX_SMALL - offset); i++) {
        small_output[i] = small_output[i + offset];
    }
    return;
}

/*
 * Function:  append_to_small_output
 * --------------------
 * Modifies small_output (global) to append to the current_small_index (global)
 * If there is not enough room existing characters are offset to the left
 *  c: char array that is appened to small_output
 */
void append_to_small_output(char *input) {
    int len = get_length(input);
    if ((current_index_small + len - 1) > MAX_SMALL - 1) {
        int offset = current_index_small - MAX_SMALL + len;
        offset_small_output(offset);
        for (int i = 0; i < len; i++) {
            small_output[MAX_SMALL - offset + i] = input[i];
        }
    } else {
        for (int i = 0; i < len; i++) {
            small_output[current_index_small + i] = input[i];
        }
        current_index_small += len;
    }
    return;
}

/*
 * Function:  void remove_decimals
 * --------------------
 * Takes a char array a modify it inplace to remove non significant decimals
 * Uses has_dot (global)
 *  c: char array that fill be modified
 * Examples:
 *  2.00100 -> 2.001
 *  2.00 -> 2
 */
void remove_decimals(char *c) {
    int flag = 1;
    int end  = 10;
    while (flag) {
        if ((c[end] == '0') || (c[end] == ' ')) {
            c[end] = ' ';
            end--;
        } else if (c[end] == '.') {
            c[end] = ' ';
            flag   = 0;
        } else {
            flag = 0;
        }
    }
    return;
}

/*
 * Function:  perform_operation
 * --------------------
 * Calls the function from avr_f64 to perform the operation with double precision
 * If the result of the operation is NaN set the current_index_big to -1 to force a reset
 */
void perform_operation(int op, float64_t v1, float64_t v2) {
    char* string ;
    if (op == add) {
        small_value = f_add(v1, v2);
        big_value   = small_value;
        string = f_to_string(big_value, MAX_BIG-1, 2);
        memcpy(big_output, string, sizeof(big_output));
    } else if (op == subtract) {
        small_value = f_sub(v1, v2);
        big_value   = small_value;
        string = f_to_string(big_value, MAX_BIG-1, 2);
        memcpy(big_output, string, sizeof(big_output));
    } else if (op == multiply) {
        small_value = f_mult(v1, v2);
        big_value   = small_value;
        string = f_to_string(big_value, MAX_BIG-1, 2);
        memcpy(big_output, string, sizeof(big_output));
    } else if (op == divide) {
		small_value = f_div(v1, v2);
		big_value   = small_value;
		string = f_to_string(big_value, MAX_BIG-1, 2);
		memcpy(big_output, string, sizeof(big_output));
    } else if (op == enter) {
        // oh shit..
    }
	current_index_big = 0;
	if (big_value == float64_ONE_POSSIBLE_NAN_REPRESENTATION){
		current_index_big = -1;
	}
    return;
}





/*
 * Function:  get_operator_sign
 * --------------------
 * Returns the string corresponding to the operator
 */
char *get_operator_sign(int op) {
    if (op == multiply) {
        return ("*");
    } else if (op == divide) {
        return ("/");
    } else if (op == subtract) {
        return ("-");
    } else if (op == add) {
        return ("+");
    } else if (op == enter) {
        return ("=");
    } else {
        // oh shit..
        return ("!");
    }
}

/*
 * Function:  perform_enter
 * --------------------
 * Perform the operation using the big and small values
 * Store the big value into the right value (in case = is called again)
 *
 */
void perform_enter(void) {
    if (has_dot) remove_decimals(big_output);
    big_value           = f_strtod(big_output, &nullptr);
    right_value.content = big_value;
    memcpy(right_value.str, big_output, sizeof(big_output));
    right_value.validity = 1;
    append_to_small_output(big_output);
    append_to_small_output(get_operator_sign(enter));
    current_index_big   = 0;
    current_index_small = 0;
    has_dot             = false;
    perform_operation(operator, small_value, big_value);
    return;
}

/*
 * Function:  process_operator
 * --------------------
 * Processes [*+-/]
 *  op: enum of the operator
 *
 */
void process_operator(int op) {
    // after NaN do nothing
    if (current_index_big < 0) {
        return;
    }
	// if no is no previous operator takes the big value and puts into the small value
	// and append the operator at the end while storing its value 
    if (operator == empty) {
        operator = op;
        if (has_dot) remove_decimals(big_output);
        big_value   = f_strtod(big_output, &nullptr);
        small_value = big_value;
        strcpy(small_output, big_output);
        current_index_small = get_length(big_output);
        append_to_small_output(get_operator_sign(op));
        right_value.validity = 0;
        current_index_big    = 0;
        has_dot              = false;
    } else {
        // if last input was also an operator
        if (is_op) {
            if (op == enter) {
                if (right_value.validity) {
                    memcpy(small_output, reset_small, sizeof(reset_small));
                    current_index_small = 0;
                    append_to_small_output(big_output);
                    append_to_small_output(get_operator_sign(operator));
                    append_to_small_output(right_value.str);
					append_to_small_output("=");
                    perform_operation(operator, big_value, right_value.content);
                    current_index_small = 0;
                } else {
					if (operator != enter){
                        perform_enter();
                    }else{
                        return;
                    }
                }
            } else {
                if (right_value.validity) {
                    right_value.validity = 0;
                    operator             = op;
                    memcpy(small_output, reset_small, sizeof(reset_small));
                    append_to_small_output(big_output);
                    append_to_small_output(get_operator_sign(op));
                } else {
                    // we update the last operator
                    operator = op;
                    // we change the current sign in the small output
                    // [0] to get a char
                    small_output[current_index_small - 1] = get_operator_sign(op)[0];
                }
            }
        } else {
            if (op == enter) {
                perform_enter();
            } else {
                if (has_dot) remove_decimals(big_output);
                big_value = f_strtod(big_output, &nullptr);
                right_value.validity = 0;
                append_to_small_output(big_output);
                append_to_small_output(get_operator_sign(op));
                current_index_big = 0;
                has_dot           = false;
                operator = op;
            }
        }
    }
    is_op = true;
    return;
}

void set_small_value(void) { small_value = f_strtod(big_output, NULL); }

/*
 * Function:  process_value
 * --------------------
 * Processes [0-9]
 * If 0 checks if the current value is different than "0"
 * Check if there is still room
 * 	if yes appends the value
 * 	else do nothing
 */
void process_value(char value) {
    if (operator == enter){
        current_index_small = 0;
    }
    if (value == '0') {
        if (current_index_big < 10) {
            if (current_index_big <= 0) {
                memcpy(big_output, reset_big, sizeof(reset_big));
                current_index_big = 0;
            }
            big_output[current_index_big] = value;
            current_index_big++;
        }
    } else {
        // if there is still room
        if ((current_index_big < 10) && (current_index_big >= 0)) {
            is_op = false;
            if ((current_index_big == 1) && (big_output[0] == '0')) {
                big_output[0] = value;
            } else {
                if (current_index_small == 0) {
                    memcpy(small_output, reset_small, sizeof(reset_small));
                    operator = empty;
                }
                if (current_index_big == 0) {
                    memcpy(big_output, reset_big, sizeof(reset_big));
                }
                big_output[current_index_big] = value;
                current_index_big++;
            }
        } else if (current_index_big < 0) {
            reset();
            big_output[current_index_big] = value;
            current_index_big++;
        }
        // else do nothing
    }
}

/*
 * Function:  process_dot
 * --------------------
 * If there is no dot:
 * 	if at the start of the output add 0.
 * 	else add a dot to the current output
 *
 */
void process_dot(void) {
    if (!has_dot) {
        if ((current_index_big < 11) && (current_index_big > 0)) {
            big_output[current_index_big] = '.';
            current_index_big++;
            is_op   = false;
            has_dot = true;
        } else if (current_index_big < 11) {
            big_output[current_index_big] = '0';
            current_index_big++;
            big_output[current_index_big] = '.';
            current_index_big++;
            is_op   = false;
            has_dot = true;
        } else {
            // do nothing
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_CALC:
            if (record->event.pressed) {
                reset();
            }
            break;
        case KC_PDOT:
            if (record->event.pressed) {
                process_dot();
            }
            break;
        case KC_P0:
            if (record->event.pressed) {
                process_value('0');
            }
            break;
        case KC_P1:
            if (record->event.pressed) {
                process_value('1');
            }
            break;
        case KC_P2:
            if (record->event.pressed) {
                process_value('2');
            }
            break;
        case KC_P3:
            if (record->event.pressed) {
                process_value('3');
            }
            break;
        case KC_P4:
            if (record->event.pressed) {
                process_value('4');
            }
            break;
        case KC_P5:
            if (record->event.pressed) {
                process_value('5');
            }
            break;
        case KC_P6:
            if (record->event.pressed) {
                process_value('6');
            }
            break;
        case KC_P7:
            if (record->event.pressed) {
                process_value('7');
            }
            break;
        case KC_P8:
            if (record->event.pressed) {
                process_value('8');
            }
            break;
        case KC_P9:
            if (record->event.pressed) {
               process_value('9');
            }
            break;
        case KC_PENT:  // enter
            if (record->event.pressed) {
                process_operator(enter);
             }
            break;
        case KC_PPLS:  // add
            if (record->event.pressed) {
                process_operator(add);
            }
            break;
        case KC_PAST: // multiply
            if (record->event.pressed) {
                process_operator(multiply);
            }
            break;
        case KC_PMNS: // subtract
            if (record->event.pressed) {
                process_operator(subtract);
            }
            break;
        case KC_PSLS: // divide
            if (record->event.pressed) {
                process_operator(divide);
            }
            break;
		default:
			return true;
    }
    render();
    return true;
};
