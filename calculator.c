#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define true 1
#define false 0

enum operators { enter, add, subtract, multiply, divide, empty };
int has_dot = false;  // Is there already a dot in big_value?
int positive = true;  // sign of big_output
int               current_index_big   = 0;  // current position in the big output + indicator for 0 division
unsigned int      current_index_small = 0;  // current position in the small output
const int MAX_SMALL = 22;
static const char reset_big[12]       = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
static const char reset_small[23]     = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
char              big_output[12]      = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
char              small_output[23]    = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
int               operator            = empty;
int               is_op               = false;  // flag to know if last action was an operator (excluding =)
char *nullptr;
double small_value = 0;  // computed value in small area
double big_value   = 0;  // value currently being displayed
struct RightValue {
    double content;
    char   str[12];
    int    validity;
};
struct RightValue right_value = {0.0, "", 0};
const long int    max_value   = 99999999999;
const long int    min_value   = -9999999999;

void render(void) {
    // render_operator();
    // render_small_value();
    // render_big_value();
    // puts("TO DO");
    printf("Small output: %s\n", small_output);
    printf("Big output: %s\n", big_output);
    printf("has_dot: %i\n", has_dot);
    printf("operator: %i\n", operator);
    printf("small_value: %f %x\n", small_value, &right_value);
    printf("big_value: %f %x\n", big_value, &big_value);
    printf("has_right_value: %i\n", right_value.validity);
    printf("cib: %i\n", current_index_big);
    printf("cis: %u\n", current_index_small);
    printf("is_op: %i\n", is_op);
    return;
}

void reset(void) {
    current_index_big   = 0;
    current_index_small = 0;
    memcpy(small_output, reset_small, sizeof(reset_small));
    memcpy(big_output, reset_big, sizeof(reset_big));
    has_dot              = false;
    operator             = empty;
    positive             = true;
    right_value.validity = 0;
    is_op = false;
    // oled_clear();
    // oled_write("CALC READY", false);
    printf("Calc ready");
    return;
}

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

void offset_small_output(int offset, int len){
    for (int i=0; i<(MAX_SMALL-offset); i++){
        small_output[i] = small_output[i + offset];
    }
    return;
}

void append_to_small_output(char* input){
    int len = get_length(input);
    if ((current_index_small + len - 1) > MAX_SMALL - 1){
        int offset = current_index_small - MAX_SMALL + len;
        offset_small_output(offset, len);
        for (int i=0; i<len;i++){
            small_output[MAX_SMALL - offset + i] = input[i];
        }
    }else{
        for (int i=0; i<len;i++){
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
 * Uses the global variable has_dot
 *  c: char array that fill be modified
 * Examples:
 *  2.00100 -> 2.001
 *  2.00 -> 2
 */
void remove_decimals(char *c) {
    int flag = 1;
    int end  = 10;
    while (flag) {
        if ((c[end] == '0') || (c[end] == ' ')){
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
 * Function:  void format_output
 * --------------------
 * Takes a char array a modify it inplace to remove non significant decimals
 * Examples:
 *  2.00100 -> 2.001
 *  2.00 -> 2
 */
void format_output(char *c, double value) {
    if (value > max_value) {
        sprintf(c, "%.5E", value);
    } else if (value < min_value) {
        sprintf(c, "%.4E", value);
    } else {
        snprintf(c, 12, "%.9f", value);
        int has_dot = 0;
        int i       = 0;
        while ((has_dot == 0) && (i < 11)) {
            if (c[i] == '.') {
                has_dot = i;
            }
            i++;
        }
        if (has_dot > 0) {
            int flag = 1;
            int end  = 10;
            while (flag) {
                if (c[end] == '0') {
                    c[end] = ' ';
                    end--;
                } else if (c[end] == '.') {
                    c[end] = ' ';
                    flag   = 0;
                } else {
                    flag = 0;
                }
            }
        }
    }
    return;
}

void cannot_divide_by_zero(void) {
    reset();
    //   oled_clear();
    strcpy(big_output, "NO ZERO DIV");
    printf("CANNOT DIVIDE BY ZERO\n");
    //   oled_write("CANNOT DIVIDE BY ZERO");
    current_index_big   = -1;
    current_index_small = 0;
    memcpy(small_output, reset_small, sizeof(reset_small));
    has_dot              = false;
    operator             = empty;
    positive             = true;
    right_value.validity = 0;
    is_op = false;
    return;
}

/*
 * Function:  perform_operation
 * --------------------
 * TODO
 *  op: TODO
 *  small_value: TODO
 *  big_value: TODO
 *
 */
void perform_operation(int op, double v1, double v2) {
    // we use the globally stored operator
    // we set small_value to be
    if (op == add) {
        small_value = v1 + big_value;
        big_value   = small_value;
        format_output(big_output, big_value);
    } else if (op == subtract) {
        small_value = v1 - v2;
        big_value   = small_value;
        format_output(big_output, big_value);
    } else if (op == multiply) {
        small_value = v1 * v2;
        big_value   = small_value;
        format_output(big_output, big_value);
    } else if (op == divide) {
        if (v2 == 0) {
            cannot_divide_by_zero();
            return;
        } else {
            small_value = v1 / v2;
            big_value   = small_value;
            format_output(big_output, big_value);
        }
    } else if (op == enter) {
        printf("ERROR\n");
    }
    current_index_big = 0;
    return;
}

void set_small_output(void) {
    // if current length < 22 then justify to the right
    // else start removing from the left
}

char* get_operator_sign(int op) {
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
        // nver goes here
        return ("!");
    }
}


/*
 * Function:  perform_enter
 * --------------------
 * TODO
 *  op: TODO
 *  small_value: TODO
 *  big_value: TODO
 *
 */
void perform_enter(void) {
    if (has_dot) remove_decimals(big_output);
    big_value           = strtod(big_output, &nullptr);
    right_value.content = big_value;
    memcpy(right_value.str, big_output, sizeof(big_output));
    right_value.validity = 1;
    append_to_small_output(big_output);
    append_to_small_output(get_operator_sign(enter));
    current_index_big                 = 0;
    current_index_small               = 0;
    has_dot                           = false;
    perform_operation(operator, small_value, big_value);
    return;
}

/*
 * Function:  process_operator
 * --------------------
 * TODO
 *  op: TODO
 *  small_value: TODO
 *  big_value: TODO
 *
 */
void process_operator(int op) {
    // if after a 0 division
    if (current_index_big < 0){
      return;
    }
    if (operator== empty) {
        operator= op;
        if (has_dot) remove_decimals(big_output);
        big_value   = strtod(big_output, &nullptr);
        small_value = big_value;
        strcpy(small_output, big_output);
        current_index_small               = get_length(big_output);
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
                    perform_operation(operator, right_value.content, big_value);
                    current_index_small = 0;
                } else {
                    perform_enter();
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
                    operator= op;
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
                big_value                         = strtod(big_output, &nullptr);
                //small_value                       = big_value;
                right_value.validity              = 0;
                append_to_small_output(big_output);
                append_to_small_output(get_operator_sign(op));
                current_index_big = 0;
                has_dot           = false;
                perform_operation(operator, small_value, big_value);
                operator= op;
            }
        }
    }
    is_op = true;
    return;
}

void set_small_value(void) { small_value = strtod(big_output, NULL); }

/*
 * Function:  process_value
 * --------------------
 * TODO
 *
 */
void process_value(char value) {
    if (value == '0') {
        if (current_index_big < 10) {
            if (current_index_big <= 0){
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
        }else if (current_index_big < 0){
          memcpy(big_output, reset_big, sizeof(reset_big));
          current_index_big = 0;
          big_output[current_index_big] = value;
          current_index_big++;
        }
        // else do nothing
    }
}

/*
 * Function:  process_dot
 * --------------------
 * TODO
 *
 */
void process_dot(void) {
    if (!has_dot) {
        if ((current_index_big < 11) && (current_index_big > 0)) {
            printf("no dot, 0<cib<11\n");
            big_output[current_index_big] = '.';
            current_index_big++;
            is_op   = false;
            has_dot = true;
        } else if (current_index_big < 11) {
            printf("no dot, cib=0\n");
            big_output[current_index_big] = '0';
            current_index_big++;
            big_output[current_index_big] = '.';
            current_index_big++;
            is_op   = false;
            has_dot = true;
        } else {
            printf("no dot, cib>11\n");
            // do nothing
        }
    }
}

// void set_small_display(int operator, double small_value, double big_value, bool full_operation){
//   if (full_operation){
//     strncpy(small_output, "T", 23); // TO DO
//   }else{
//     char small_output[22];
//     sprintf(small_output, "%f", small_value);
//     int start = 83;
//     for (int i = strlen(small_output)-1; i >= 0; i++){
//       output[start] = small_output[i];
//       start --;
//     }
//   }
// }

void reset_big_output(void) { return; }

void process_record_user(int keycode) {
    switch (keycode) {
        case 'c':
            reset();
            break;
        case 'n':
            printf("numlock");
            reset();
            break;
        case '.':
            process_dot();
            break;
        case '0':
            process_value('0');
            break;
        case '1':
            process_value('1');
            break;
        case '2':
            process_value('2');
            break;
        case '3':
            process_value('3');
            break;
        case '4':
            process_value('4');
            break;
        case '5':
            process_value('5');
            break;
        case '6':
            process_value('6');
            break;
        case '7':
            process_value('7');
            break;
        case '8':
            process_value('8');
            break;
        case '9':
            process_value('9');
            break;
        case '=':  // enter
            process_operator(enter);
            break;
        case '+':  // plus
            process_operator(add);
            break;
        case '/':
            process_operator(divide);
            break;
        case '-':
            process_operator(subtract);
            break;
        case '*':
            process_operator(multiply);
            break;
    }
    // return true;
    render();
    return;
};

void loop(int c) {
    printf("Type:");
    c = getchar();
    printf("Typed: %i\n", c);
    process_record_user(c);
    printf("\n");
    loop(c);
}

int main(void) {
    int c;
    loop(c);
    return (0);
}
