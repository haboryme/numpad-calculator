# zz15z3oled

A numpad with a calculator.


Built on a Pro Micro (Atmega32u4) and paired with a 124x68 OLED screen.
avr-gcc doesn't support the double type, I used the avr_64f library to emulate double precision using uint64_t.
I removed some lines of code from avr64_f, you can find the original post from the author along with some interesting discussions (ing german) here : https://www.mikrocontroller.net/topic/85256.
With the current settings overflow leads to NaN and underflow to 0.

All the logics for the calculator itself are coded in the keymap.c file. 
It follows the behaviour of the Windows (10) calculator.
Only divide, subtract, add and multiply are currently supported.


