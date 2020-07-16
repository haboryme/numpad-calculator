# zz15z3oled  

A numpad with a calculator and a 3D printed case 

The calculator itself:
Built on a Pro Micro (Atmega32u4) and paired with a 124x68 OLED screen.  
avr-gcc doesn't support the double type, I used the avr_64f library to emulate double precision using uint64_t.  
I removed some lines of code from avr_f64, you can find the original post from the author along with some interesting discussions (in german) here : https://www.mikrocontroller.net/topic/85256.  
With the current settings overflow leads to NaN and underflow to 0.  

All the logics for the calculator itself are coded in the keymap.c file.   
It follows the behaviour of the Windows (10) calculator. 
Only divide, subtract, add and multiply are currently supported.  

The board:
4 parts:
- case: 3D printed
- plate: laser cut metal (1.5mm)
- weight (optional): laser cut metal (1.5mm)
- promicro holder: 3D printed
- promicro holder lock: 3D printed

All printing was performed on an Ender 3 with Creality PLA at 215°C, with 0.12mm layers and with supports (simplify3d as slicer).
4.4mm Ø and 3.1mm deep holes should be fitted with M3 brass inserts.
The front circular opening in the case should be fitted with a LEMO 0B femal connector (at least 4 pins).
Only M3 screws are used.

There is NO PCB: this is a handwired build, all switches should be fitted with a diode and connected to the promicro in a column/row fashion (update the configuration file for QMK according to the actual wiring).

It's possible to use a 4-pin JST connector for the screen so that it can easily be disconnected and the plate removed (on the side of the screen keep 1-pin connector that can fit through the opening in the plate).

Files:  
glcfont.c - contains the tileset used for displaying characters on the OLED screen, it can be visualized with https://helixfonteditor.netlify.app/  
rules.mk  
config.h - some optimization available here  
zz15z3oled.c  
zz15z3oled.h - defines the base keymap  
keymaps/default/rules.mk - it must have the SRC += avr_f64.c line so that the compiler knows how to link it.  
keymaps/default/avr_f64.c  
keymaps/default/avr_f64.h  
models/numpad case.stl - 3d model for the case
models/numpad plate.dxf - 2d vector file for the plate
models/numpad weight.dxf - 2d vector file for the weight
models/promicro holder lock.stl - 3d model of the lock that fits on the holder
models/promicro holder.stl - 3d model of the holder
numpad 1.0.skp - sketchup file with all the components available for modification
