#A CHIP-8 emulator written in C
--------------------------------
The `CHIP-8` isn't an actual computer, but a language interpreter used on computers from the '70s and '80s, primarily the RCA TELMAC 1800 and the COSMAC VIP, both based on the CDP-1802 CPU. Hobbyist computers saw a fair amount of use. It was mostly used for making programming video games easier. In the '90s, it saw widespread use on calculators as a programming tool for, again, developing video games.

For all intents and purposes, however, it's a complete system. A "virtual computer" if you will, and like any other computer can be emulated, given opcodes.

This is an emulator for the CHIP-8, written in C (using Mark Kilgard's windowing system independent openGL library, GLUT, for graphics/input/sound)

#Progress
---------
###01-14-2016
* Implemented graphics. Major bug with display array; draws garbage.
* Still no input/audio.

###01-12-2016
* Right now this project is nothing more than a debugger with a hardcoded ROM.


#Compilation
------------
For Windows (Visual Studio), you'll need Nate Robins' Win32 GLUT port, properly linked.
* glut.h      in your Includes directory
* glut32.dll  in your project .exe dir (or system32, but that's unnecessary)
* glut32.lib  in your Libary directory


For Linux, I guess you'll just need a GLUT implementation like freeglut3 (and freeglut3-dev)? I'm not very sure.
Compile like : 
* cc	chip8core.c 	-lglut 	-lGL 	-lGLU 	-lm -o 	c8
