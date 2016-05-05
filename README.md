#A CHIP-8 emulator written in C
--------------------------------
The `CHIP-8` isn't an actual computer, but a language interpreter used on computers from the '70s and '80s, primarily the RCA TELMAC 1800 and the COSMAC VIP, both based on the CDP-1802 CPU. While hobbyist computers saw a fair amount of use, it was mostly used for making programming video games easier. In the '90s, it saw widespread use on calculators as a programming tool for, again, developing video games.

For all intents and purposes, however, it's a complete system. A "virtual computer" if you will, and like any other computer can be emulated, given opcodes.

This is an emulator for the CHIP-8, written in C (using Mark Kilgard's windowing system independent openGL library, GLUT, for graphics/input/sound). Screenshots below.

###Brix

![Brix](https://cloud.githubusercontent.com/assets/8657811/15047354/33d97df0-1302-11e6-8a38-742ac029495e.jpg)


###Space Invaders

![Space Invaders](https://cloud.githubusercontent.com/assets/8657811/15047355/33da60a8-1302-11e6-8e89-d89845110682.jpg)


###Pong

![Pong](https://cloud.githubusercontent.com/assets/8657811/15047356/33dab940-1302-11e6-8b60-4cee424fac31.jpg)

#Progress
---------
###05-04-2016
* Implented working keyboard input, a mapped hex keyboard.
* Fixed a tonne of bugs. Most games/demo ROMs work without issues now.
* Still no audio
 
###04-30-2016
* Fixed display bug. Didn't test beyond demos, however.
* Still no input/audio.

###01-14-2016
* Implemented graphics. Major bug with display array; draws garbage.
* Still no input/audio.

###01-12-2016
* Right now this project is nothing more than a debugger with a hardcoded ROM.


#Usage
-------
\# ROMs need to be in the same directory as the executable. Right now, the path is hardcoded. Comment/uncomment as necessary.

\# `ESC` button quits the emulator. Closing the OpenGL window does the same.

\# The hexadecimal (0-F) keypad of the original CHIP-8 is mapped on a modern keyboard as follows. Note that keys 2-4-6-8 (original)/2-Q-E-S (mapped) are used for directional control (arrow keys) in most games.



###ORIGINAL:
--------

`1 2 3 C`

`4 5 6 D`

`7 8 9 E`

`A 0 B F`


###MAPPED:
--------

`1 2 3 4`

`Q W E R`

`A S D F`

`Z X C V`


#Requirements
-------------
A GPU (with its installed driver, of course) that supports OpenGL 1.1. This likely won't be an issue on modern systems.


#Provided ROMs
--------------
Three demos and three games, in .ch8 format. I'm not sure of their availability in the public domain, and will remove them if they are discovered to, in fact, not be as such.


#Compilation
------------
For Windows (Visual Studio), you'll need Nate Robins' Win32 GLUT port, properly linked.
* glut.h      in your Includes directory
* glut32.dll  in your project .exe dir (or system32, but that's unnecessary)
* glut32.lib  in your Libary directory


For Linux, I guess you'll just need a GLUT implementation like freeglut3 (and freeglut3-dev)? I'm not very sure.
Compile like : 
* cc	chip8core.c 	-lglut 	-lGL 	-lGLU 	-o 	c8
